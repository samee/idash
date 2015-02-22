#pragma once
#define LOC_NAME_MAX 30
#include<stdio.h>

typedef struct LocSum { char loc[LOC_NAME_MAX]; int x,y; } LocSum;
typedef struct FileDetails { FILE* fp; int sampCount; } FileDetails;
LocSum nextRecord(FileDetails* args);

// Only works for semi-honest
typedef struct ProtocolArgs 
{ FileDetails fcase,fctrl;
} ProtocolArgs;

void chi2_file(void* arg);
