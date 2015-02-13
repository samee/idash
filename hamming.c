#include<stdbool.h>
#include<stdio.h>
#include<gcrypt.h>
#include<obliv.h>
#include<obliv_common.h>
#include"hamming.h"

char* programName;
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
void vcfSkipLine(FILE* fp)
{ char buf[FILE_LINE_BUF_LEN];
  int len;
  do
  { if(!fgets(buf,sizeof(buf),fp)) break;
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

bool loadVcfFile(HashType** phashes,size_t* psz,const char* filename)
{
  HashType* hashes = malloc(MAX_HASHES*HASH_BYTES);
  int i,sz=0;
  char line[FILE_LINE_BUF_LEN];
  if(!hashes) out_of_memory_exit();

  FILE* fp = fopen(filename,"r");
  if(!fp) { free(hashes); return false; }

  gcryDefaultLibInit(); // I should probably expose/prefix this function
  gcry_md_hd_t hasher;
  gcry_error_t gcry_err = gcry_md_open(&hasher,GCRY_MD_SHA1,0);
  if(!hasher) gcrypt_error_exit(gcry_err);

  sz=0;
  unsigned char dtChrom;
  long long dtPos;
  char dtAlt[FILE_MAX_LINE_LEN];
  while(!feof(fp))
  { if(vcfSkipCommentLine(fp)) continue;
    vcfParseLine(fp,line,dtChrom,dtPos,dtAlt); // ignores REF field //-
    vcfHashAndReset(hasher,hashes[sz], dtChrom, dtPos, dtAlt); //-
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

int main(int argc,char* argv[])
{
  programName = argv[0];
  if((argv[2][0]=='1' && argc!=4) || (argv[2][0]=='2' && argc!=5))
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
