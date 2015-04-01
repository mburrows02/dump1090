#include <stdio.h>
#include <tidy/tidy.h>
#include <tidy/buffio.h>
#include <curl/curl.h>

struct flight {
  char *dep;
  char *arr;
};

uint write_cb(char *in, uint size, uint nmemb, TidyBuffer *out);
int findElement(TidyNode tnod, TidyTagId tagId, int checkAttr, TidyAttrId attrId, char *attrVal, TidyNode *outNode);
void findInfo(TidyDoc tdoc, char **flightSrc, char **flightDst);
int getFlightInfo(char *flightNumber, char **flightSrc, char **flightDst);