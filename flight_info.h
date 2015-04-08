#include <stdio.h>
#include <tidy/tidy.h>
#include <tidy/buffio.h>
#include <curl/curl.h>
#include "dump1090.h"

uint write_cb(char *in, uint size, uint nmemb, TidyBuffer *out);
int findElement(TidyNode tnod, TidyTagId tagId, int checkAttr, TidyAttrId attrId, char *attrVal, TidyNode *outNode);
char *getElementText(TidyDoc tdoc, TidyNode txtNode);
void getInfo(TidyDoc tdoc, struct aircraft *a);
void getLocations(TidyNode trackTableBody, struct aircraft *a);
void getScheduledTimes(TidyDoc tdoc, TidyNode trackTableBody, struct aircraft *a);
void getTimes(TidyDoc tdoc, TidyNode trackTableBody, struct aircraft *a);
int getFlightInfo(struct aircraft *a);