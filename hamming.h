#pragma once

#ifndef MAX_HASHES
#define MAX_HASHES 100000
#endif

#ifndef HASH_BYTES
#define HASH_BYTES 8
#endif

#ifndef FILE_MAX_LINE_LEN
#define FILE_MAX_LINE_LEN 200
#endif

// Null terminator, and a newline preceding it, since we may use fgets()
#define FILE_LINE_BUF_LEN (FILE_MAX_LINE_LEN+2)


typedef char HashType[HASH_BYTES];

void unionCount(void* arg);

typedef struct
{ HashType* data;
  size_t datasz;
  size_t unionCount; // result
} UnionCountIO;
