#include "flight_info.h"
 
/* curl write callback, to fill tidy's input buffer...  */ 
uint write_cb(char *in, uint size, uint nmemb, TidyBuffer *out)
{
  uint r;
  r = size * nmemb;
  tidyBufAppend( out, in, r );
  return(r);
}
 
/* Traverse the document tree (DFS) */ 
int findElement(TidyNode tnod, TidyTagId tagId, int checkAttr, TidyAttrId attrId, char *attrVal, TidyNode *outNode)
{
  TidyNode child;
  for (child = tidyGetChild(tnod); child; child = tidyGetNext(child) )
  {
    /* Check tag type */
    if (tidyNodeGetId(child) == tagId )
    {
      /* Get and check desired attribute, or return the node if we only care about the tag type */
      if (checkAttr) {
        TidyAttr attr = tidyAttrGetById(child, attrId);
        ctmbstr val = tidyAttrValue(attr);
        if (!strcmp(val, attrVal)) {
          FILE *fp = fopen("./log.txt", "ab");
          if (fp != NULL) {
            fputs(val, fp);
            fputs("\n", fp);
            fclose(fp);
          }
          *outNode = child;
          return 0;
        }
      } else {
        *outNode = child;
        return 0;
      }
    }
    if (findElement(child, tagId, checkAttr, attrId, attrVal, outNode) == 0) {
      return 0;
    } 
  }
  return 1;
}

void findInfo(TidyDoc tdoc, char **flightSrc, char **flightDst) 
{
  FILE *fp = fopen("./log.txt", "ab");
  TidyNode root = tidyGetRoot(tdoc);
  TidyNode trackTable, trackTableBody, locationsRow, depCell, depAirportDiv, arrCell, arrAirportDiv;
  //Find the flight information table
  findElement(root, TidyTag_TABLE, 1, TidyAttr_CLASS, "track-panel-course", &trackTable);
  findElement(trackTable, TidyTag_TBODY, 0, 0, 0, &trackTableBody);
  findElement(trackTableBody, TidyTag_TR, 0, 0, 0, &locationsRow);
  //Get the departure information
  findElement(locationsRow, TidyTag_TD, 1, TidyAttr_CLASS, "track-panel-departure", &depCell);
  findElement(depCell, TidyTag_DIV, 1, TidyAttr_CLASS, "track-panel-terminal", &depAirportDiv);
    fputs("Hello1\n", fp);
  TidyBuffer buf1;
  tidyBufInit(&buf1);
  tidyNodeGetText(tdoc, tidyGetChild(depAirportDiv), &buf1);
  fputs("Hello2\n", fp);
  char *src = (char *) malloc(buf1.size);
  fputs("Hello3\n", fp);
  strcpy(src, (char *)buf1.bp);
  fputs("Hello4\n", fp);
  tidyBufFree(&buf1);
    fputs(src, fp);
    fputs("\n", fp);
  *flightSrc = src;

  //Get the arrival information
  findElement(locationsRow, TidyTag_TD, 1, TidyAttr_CLASS, "track-panel-arrival", &arrCell);
  findElement(arrCell, TidyTag_DIV, 1, TidyAttr_CLASS, "track-panel-terminal", &arrAirportDiv);
  TidyBuffer buf2;
  tidyBufInit(&buf2);
  tidyNodeGetText(tdoc, tidyGetChild(arrAirportDiv), &buf2);
  char *dst = (char *) malloc(buf2.size);
  strcpy(dst, (char *)buf2.bp);
  tidyBufFree(&buf2);
  *flightDst = dst;
  fclose(fp);
}

int getFlightInfo(char *flightNumber, char **flightSrc, char **flightDst)
{
  char *urlStart = "http://flightaware.com/live/flight/";
  char *url = (char*) malloc(strlen(flightNumber) + strlen(urlStart));
  strcpy(url, urlStart);
  strcat(url, flightNumber);
  FILE *fp = fopen("./log.txt", "ab");
  if (fp != NULL) {
    fputs(url, fp);
    fputs("\n", fp);
    fclose(fp);
  }
  CURL *curl;
  char curl_errbuf[CURL_ERROR_SIZE];
  TidyDoc tdoc;
  TidyBuffer docbuf = {0};
  TidyBuffer tidy_errbuf = {0};
  int err;
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_errbuf);
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
  //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);

  tdoc = tidyCreate();
  tidyOptSetBool(tdoc, TidyForceOutput, yes); /* try harder */ 
  tidyOptSetInt(tdoc, TidyWrapLen, 4096);
  tidySetErrorBuffer( tdoc, &tidy_errbuf );
  tidyBufInit(&docbuf);

  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &docbuf);
  err=curl_easy_perform(curl);
  if ( !err ) {
    err = tidyParseBuffer(tdoc, &docbuf); /* parse the input */ 
    if ( err >= 0 ) {
      err = tidyCleanAndRepair(tdoc); /* fix any problems */ 
      if ( err >= 0 ) {
        err = tidyRunDiagnostics(tdoc); /* load tidy error buffer */ 
        if ( err >= 0 ) {
          findInfo(tdoc, flightSrc, flightDst);
          //fprintf(stderr, "%s\n", tidy_errbuf.bp); /* show errors */ 
        }
      }
    }
  }
  else
    fprintf(stderr, "%s\n", curl_errbuf);

  /* clean-up */ 
  curl_easy_cleanup(curl);
  tidyBufFree(&docbuf);
  tidyBufFree(&tidy_errbuf);
  tidyRelease(tdoc);
  return(err);

}
 
 
/*int main(int argc, char **argv )
{
  char *flightSrc = (char *) malloc(BUFF_SIZE);
  char *flightDst = (char *) malloc(BUFF_SIZE);
  int err = getFlightInfo("ANT512", &flightSrc, &flightDst);
  printf("Departure: %s\n", flightSrc);
  printf("Arrival: %s\n", flightDst);
  return err;
}*/