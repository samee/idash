#include<assert.h>
#include<stdbool.h>
#include<stdio.h>
#include<string.h>
#include<gcrypt.h>
#include<obliv.h>
#include<obliv_common.h>
#include"hamming.h"
#include"util.h"

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
// Assumes s is a null-terminated string of length at most 3
// Returns -1 on parse error
int vcfParseChrom(const char* s)
{
  int rv;
  switch(s[0])
  { case 'X': return 112;
    case 'Y': return 113;
    case 'M': return 111;
    default: if(sscanf(s,"%d",&rv)==1) return rv; else return -1;
  }
}
bool vcfParseLine(FILE* fp,unsigned char* chrom,long long* pos,char alt[])
{
  char line[FILE_MAX_LINE_LEN],schrom[4];
  if(!fgets(line,sizeof(line),fp)) file_parse_error_exit();
  if(strstr(line,"INS") || strstr(line,"DEL")) return false;
  fileLoc++;
  int len=strlen(line),ichrom;
  if(line[len-1]!='\n') file_parse_error_exit();
  if(sscanf(line,"%3s %lld %*s %*s %s",schrom,pos,alt)<3)
    file_parse_error_exit();
  ichrom=vcfParseChrom(schrom);
  if(ichrom<0) file_parse_error_exit();
  *chrom=ichrom;
  return true;
}
bool loadVcfFile(HashType** phashes,size_t* psz,const char* filename)
{
  HashType* hashes = malloc(2*MAX_HASHES*HASH_BYTES);
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
    if(!vcfParseLine(fp,&dtChrom,&dtPos,dtAlt)) continue;
    vcfHashAndReset(hasher, &hashes[sz  ], dtChrom, dtPos, dtAlt);
    vcfHashAndReset(hasher, &hashes[sz+1], dtChrom, dtPos, "");
    sz+=2;
  }
  gcry_md_close(hasher);
  fclose(fp);
  hashes = realloc(hashes,sz*HASH_BYTES); // free unused memory
  *phashes = hashes; *psz = sz;
  return true;
}
void freeFileData(HashType* hashes,int sz) { free(hashes); }

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
size_t xchgSize(ProtocolDesc* pd,int party,size_t d)
{
  size_t d2;
  if(party==1) 
  { osend(pd,2,&d ,sizeof(d));
    orecv(pd,2,&d2,sizeof(d));
  }else
  { orecv(pd,1,&d2,sizeof(d));
    osend(pd,1,&d ,sizeof(d));
  }
  return d2;
}
char **nestedArrayPtr(HashType* data,int n)
{
  char **rv = malloc(n*sizeof(HashType*));
  int i;
  for(i=0;i<n;++i) rv[i]=data[i];
  return rv;
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
  size_t mysize = io.datasz, othersize;
  othersize = xchgSize(&pd,party,mysize);
  char **outer = nestedArrayPtr(io.data,mysize);
  size_t ix=execPsiSizeProtocol_DH(&pd,outer,mysize,othersize,HASH_BYTES);
  printf("Result: %zd\n",(mysize+othersize)/2-ix);
  free(outer);
  cleanupProtocol(&pd);
  return 0;
}
