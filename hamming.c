#include<assert.h>
#include<stdbool.h>
#include<stdio.h>
#include<string.h>
#include<gcrypt.h>
#include<obliv.h>
#include<obliv_common.h>
#include"hamming.h"

char* programName;
int fileLoc;
void out_of_memory_exit()
{
  fprintf(stderr,"%s: out of memory\n",programName);
  exit(-1);
}
void gcrypt_error_exit(gcry_error_t err)
{
  fprintf(stderr,"%s: Libgcrypt error: %s\n",programName,gcry_strerror(err));
  exit(-1);
}
#define file_parse_error_exit() file_parse_error_exit_fun(__LINE__)
void file_parse_error_exit_fun(int line)
{
  fprintf(stderr,"Problem parsing file at " __FILE__ ":%d, input line: %d\n",
      line,fileLoc);
  exit(3);
}
void vcfSkipLine(FILE* fp)
{ char buf[FILE_LINE_BUF_LEN];
  int len;
  do
  { if(!fgets(buf,sizeof(buf),fp)) break;
    fileLoc++;
    len=strlen(buf);
  } while(!feof(fp) && buf[len-1]!='\n');
}
int fpeek(FILE* fp)
{ int c = fgetc(fp);
  ungetc(c,fp);
  return c;
}
bool vcfSkipCommentLine(FILE* fp)
{
  if(fpeek(fp)=='#') { vcfSkipLine(fp); return true; }
  else return false;
}

void vcfHashAndReset(gcry_md_hd_t hasher,HashType* hashDest,
                     unsigned char chrom,long long pos,const char* alt)
{
  size_t len = strlen(alt);
  assert(len<FILE_MAX_LINE_LEN);
  gcry_md_write(hasher,&chrom,sizeof(chrom));
  gcry_md_write(hasher,&pos,sizeof(pos));
  gcry_md_write(hasher,alt,len);
  assert(sizeof(HashType)<=gcry_md_get_algo_dlen(HASH_ALGO));
  memcpy(*hashDest,gcry_md_read(hasher,0),sizeof(HashType)); // truncation
  gcry_md_reset(hasher);
}

void vcfParseLine(FILE* fp,unsigned char* chrom,long long* pos,char alt[])
{
  char line[FILE_MAX_LINE_LEN];
  if(!fgets(line,sizeof(line),fp)) file_parse_error_exit();
  fileLoc++;
  int len=strlen(line),ichrom;
  if(line[len-1]!='\n') file_parse_error_exit();
  if(sscanf(line,"%d %lld %*s %*s %s",&ichrom,pos,alt)<3)
    file_parse_error_exit();
  *chrom=ichrom;
}
bool loadVcfFile(HashType** phashes,size_t* psz,const char* filename)
{
  HashType* hashes = malloc(MAX_HASHES*HASH_BYTES);
  int i,sz=0;
  if(!hashes) out_of_memory_exit();

  FILE* fp = fopen(filename,"r");
  if(!fp) { free(hashes); return false; }

  gcryDefaultLibInit(); // I should probably expose/prefix this function
  gcry_md_hd_t hasher;
  gcry_error_t gcry_err = gcry_md_open(&hasher,HASH_ALGO,0);
  if(!hasher) gcrypt_error_exit(gcry_err);

  sz=0;
  unsigned char dtChrom;
  long long dtPos;
  char dtAlt[FILE_MAX_LINE_LEN];
  fileLoc=1; // I hate globals
  while(fpeek(fp),!feof(fp))
  { if(vcfSkipCommentLine(fp)) continue;
    vcfParseLine(fp,&dtChrom,&dtPos,dtAlt); // ignores REF field
    vcfHashAndReset(hasher, &hashes[sz], dtChrom, dtPos, dtAlt);
    ++sz;
  }
  gcry_md_close(hasher);
  fclose(fp);
  hashes = realloc(hashes,sz*HASH_BYTES); // free unused memory
  *phashes = hashes; *psz = sz;
  return true;
}
void freeFileData(HashType* hashes,int sz) { free(hashes); }

void setupTcpConnection(ProtocolDesc* pd,
                        const char* remote_host,
                        const char* port)
{
  if(remote_host==NULL)
  { if(protocolAcceptTcp2P(pd,port)!=0)
    { fprintf(stderr,"TCP accept failed\n");
      exit(-1);
    }
  }
  else 
    if(protocolConnectTcp2P(pd,remote_host,port)!=0) 
    { fprintf(stderr,"TCP connect failed\n");
      exit(-1);
    }
}

void hexprint(FILE* fp,const char* s,size_t len)
{
  size_t i;
  for(i=0;i<len;++i)
    fprintf(fp,"%.02x ",0xff&s[i]);
  fprintf(fp,"\n");
}
void debugPrintHashes(UnionCountIO* io)
{
  int i;
  for(i=0;i<io->datasz;++i)
    hexprint(stdout,io->data[i],sizeof(io->data[i]));
}
int main(int argc,char* argv[])
{
  programName = argv[0];
  if(!(argv[2][0]=='1' && argc==4) && !(argv[2][0]=='2' && argc==5))
  { fprintf(stderr,"Usage: %s <input-vcf-file> 1 <port>, or\n"
                   "       %s <input-vcf-file> 2 <remote-addr> <port>\n",
                   programName,programName);
    return 1;
  }
  UnionCountIO io;
  if(!loadVcfFile(&io.data,&io.datasz,argv[1])) 
  { fprintf(stderr,"%s: could not load VCF file '%s'\n",programName,argv[3]);
    return 2;
  }
  //debugPrintHashes(&io);
  int party = (argv[2][0]=='1'?1:2);
  char *ra,*port;
  if(party==1) { ra=NULL; port=argv[3]; }
  else { ra=argv[3]; port=argv[4]; }
  ProtocolDesc pd;
  setupTcpConnection(&pd,ra,port);
  setCurrentParty(&pd,party);
  execYaoProtocol(&pd,unionCount,&io); //-
  printf("Result: %zd\n",io.unionCount);
  cleanupProtocol(&pd);
  return 0;
}
