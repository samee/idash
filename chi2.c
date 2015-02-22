#include<assert.h>
#include<stdbool.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"util.h"
#include"chi2.h"

#define file_parse_error_exit() file_parse_error_exit_fun(__LINE__)
void file_parse_error_exit_fun(int line)
{
  fprintf(stderr,"Problem parsing file at " __FILE__ ":%d\n",line);
  exit(3);
}
int baseInd(char ch)
{ switch(ch)
  { case 'A': return 0;
    case 'C': return 1;
    case 'G': return 2;
    case 'T': return 3;
    default: fprintf(stderr,"Weird base: %c\n",ch); exit(1);
  }
}

int countSamples(FILE* fp)
{
  bool inSpace=true;
  int rv=0;
  int ch;
  while(ch=fgetc(fp),ch!=EOF,ch!='\n')
  {
    bool s = isspace(ch);
    if(inSpace&&!s) ++rv;
    inSpace=s;
  }
  return rv;
}
LocSum nextRecord(FileDetails* args)
{
  LocSum rv={};
  char *s,alleles[3];
  s=fgets(rv.loc,LOC_NAME_MAX,args->fp);
  if(feof(args->fp)) return rv;
  if(!s) file_parse_error_exit();
  int locSize = strlen(s),i,sum[4]={};
  if(s[locSize-1]!='\n') file_parse_error_exit(); // line too long?
  s[locSize-1]=0;
  for(i=0;i<args->sampCount;++i) 
  {
    if(fscanf(args->fp,"%s ",alleles)<1) file_parse_error_exit();
    sum[baseInd(alleles[0])]++;
    sum[baseInd(alleles[1])]++;
  }
  i=0;
  for(;i<4;++i) if(sum[i]) { rv.x=sum[i]; break; }
  for(;i<4;++i) if(sum[i]) { rv.y=sum[i]; break; }
  return rv;
}

int main(int argc, char* argv[])
{
  const char* programName = argv[0];
  if(!(argc==5 && argv[3][0]=='1') && !(argc==6 && argv[3][0]=='2'))
  { fprintf(stderr,
      "Usage: %s <case-file> <control-file> 1 <port>, or\n"
      "       %s <case-file> <control-file> 2 <remote-addr> <port>\n",
      programName,programName);
    return 1;
  }
  FILE* fpcase = fopen(argv[1],"r");
  FILE* fpctrl = fopen(argv[2],"r");
  if(!fpcase || !fpctrl) 
  { fprintf(stderr,"Could not load %s file at '%s'.\n",
                   (fpcase?"case":"control"),
                   (fpcase?argv[1]:argv[2]));
    return 2;
  }

  FileDetails caseA = { .fp = fpcase, .sampCount = countSamples(fpcase)};
  FileDetails ctrlA = { .fp = fpctrl, .sampCount = countSamples(fpctrl)};
  assert(caseA.sampCount==ctrlA.sampCount);

  int party = (argv[3][0]=='1'?1:2);
  char *ra,*port;
  if(party==1) { ra=NULL; port=argv[4]; }
  else { ra=argv[4]; port=argv[5]; }
  ProtocolDesc pd;
  setupTcpConnection(&pd,ra,port);
  setCurrentParty(&pd,party);

  ProtocolArgs a = { .fcase=caseA, .fctrl=ctrlA };
  execYaoProtocol(&pd,chi2_file,&a);
  cleanupProtocol(&pd);
  fclose(fpcase);
  fclose(fpctrl);
  return 0;
}
