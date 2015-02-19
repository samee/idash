#include<stdlib.h>
#include<stdio.h>
#include<string.h>

const char* printChrome(int chrom)
{
  static char rv[3];
  switch(chrom)
  { case 112: strcpy(rv,"X"); break;
    case 113: strcpy(rv,"Y"); break;
    case 111: strcpy(rv,"M"); break;
    default:  sprintf(rv,"%d",chrom);
  }
  return rv;
}

void randomPos(char* chrom,int* loc)
{
  strcpy(chrom,printChrome(rand()%23+1));
  *loc=rand()%1000000000;
}

char randombase()
{
  switch(rand()%4)
  { case 0: return 'A';
    case 1: return 'C';
    case 2: return 'G';
    case 3: return 'T';
  }
  return 0;
}

typedef enum operation { SNP,SUB,INS,DEL } operation;

#define MAX_STRING_LEN 20

void randomString(char* dest)
{
  int i;
  dest[0]=randombase();
  dest[1]=randombase();
  for(i=2;rand()%2&&i<MAX_STRING_LEN;++i) dest[i]=randombase();
  dest[i]=0;
}

void randomVariant(operation* op,char* ref,char* alt)
{
  int r=rand()%5000;
  if(r<4350) *op=SNP; else { r-=4350;
  if(r< 150) *op=SUB; else { r-=150;
  if(r< 250) *op=INS; else { r-=250;
  if(r< 250) *op=DEL; else exit(-1); }}}
  if(*op==SNP)
  { char a[2]=" ",b[2]=" ";
    a[0]=randombase();
    b[0]=randombase();
    strcpy(ref,a); strcpy(alt,b);
  }
  else if(*op==INS) { randomString(alt); strcpy(ref,"."); }
  else if(*op==DEL) { randomString(ref); strcpy(alt,"."); }
  else { randomString(alt); randomString(ref); }
}

typedef struct Record 
{ char chrom[3]; int loc; 
  operation op; char ref[MAX_STRING_LEN+1], alt[MAX_STRING_LEN+1];
} Record;

void randomRecord(Record* rec)
{
  randomPos(rec->chrom,&rec->loc);
  randomVariant(&rec->op,rec->ref,rec->alt);
}

#define OUTSIZE 100000

const char* printOp(operation op)
{ static char svtype[] = "SVTYPE=   ";
  char* dest=svtype+7;
  switch(op)
  { case SNP: strcpy(dest,"SNP"); break;
    case SUB: strcpy(dest,"SUB"); break;
    case INS: strcpy(dest,"INS"); break;
    case DEL: strcpy(dest,"DEL"); break;
  }
  return svtype;
}
void printRecord(FILE* fp,Record* r)
{
  fprintf(fp,"%s\t%d\t.\t%s\t%s\t.\t.\t%s\n",r->chrom,r->loc,
             r->ref,r->alt,printOp(r->op));
}

void riffleOut(const char* filename,Record* arr1,int sz1,Record* arr2,int sz2)
{
  FILE* fp = fopen(filename,"w");
  int i=0,j=0;
  fprintf(fp,"##fileformat=VCF4.2\n"
             "##INFO=<ID=SVTYPE,Number=1,Type=String,Description=\"blah\">\n"
             "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\n"
      );
  while(i<sz1||j<sz2)
  { if(i<sz1&&(j==sz2||rand()%2)) printRecord(fp,&arr1[i++]);
    else printRecord(fp,&arr2[j++]);
  }
  fclose(fp);
}
void randomPermute(Record* arr,int sz)
{
  int i;
  for(i=1;i<sz;++i)
  { int j=rand()%i;
    Record t=arr[j];
    arr[j]=arr[i];
    arr[i]=t;
  }
}

// NOT guaranteed to produce valid genome positions
int main()
{
  srand(time(0));
  int i;
  Record *comm  = malloc(sizeof(Record)*OUTSIZE/2),
         *file1 = malloc(sizeof(Record)*OUTSIZE/2),
         *file2 = malloc(sizeof(Record)*OUTSIZE/2);
  for(i=0;i<OUTSIZE/2;++i) randomRecord(comm+i);
  for(i=0;i<OUTSIZE/2;++i) randomRecord(file1+i);
  for(i=0;i<OUTSIZE/2;++i) randomRecord(file2+i);
  riffleOut("random1.snp",comm,OUTSIZE/2,file1,OUTSIZE/2);
  randomPermute(comm,OUTSIZE/2);
  riffleOut("random2.snp",comm,OUTSIZE/2,file2,OUTSIZE/2);
  free(comm); free(file1); free(file2);
  return 0;
}
