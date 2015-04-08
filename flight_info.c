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

char *getElementText(TidyDoc tdoc, TidyNode txtNode)
{
  TidyBuffer buf;
  tidyBufInit(&buf);
  tidyNodeGetText(tdoc, txtNode, &buf);
  printf("Getting text: %s\n", buf.bp?(char *)buf.bp:"");
  if (!buf.bp) {
    return 0;
  }
  char *txt = malloc(buf.size);
  strcpy(txt, (char *)buf.bp);
  tidyBufFree(&buf);
  int i;
  for (i = 0; txt[i] != 0; ++i) {
    if (txt[i] == '\n') {
      txt[i] = ' ';
    }
  }
  return txt;
}

/* Find the tbody of the tracking panel that contains the information we want */
void getInfo(TidyDoc tdoc, struct aircraft *a)
{
  TidyNode root = tidyGetRoot(tdoc);
  TidyNode trackTable, trackTableBody, locationsRow, actualTimesRow, scheduledTimesRow;
  if (findElement(root, TidyTag_TABLE, 1, TidyAttr_CLASS, "track-panel-course", &trackTable)) {
    //Couldn't find table
    return;
  }
  findElement(trackTable, TidyTag_TBODY, 0, 0, 0, &trackTableBody);
  locationsRow = tidyGetChild(trackTableBody);
  getLocations(locationsRow, a);
  actualTimesRow = tidyGetNext(locationsRow);
  getTimes(tdoc, actualTimesRow, a);
  scheduledTimesRow = tidyGetNext(actualTimesRow);
  getScheduledTimes(tdoc, scheduledTimesRow, a);
}

/* Get the origin and destination of the flight */
void getLocations(TidyNode locationsRow, struct aircraft *a)
{
  TidyNode depCell, depAirportDiv, depHintSpan;
  TidyNode arrCell, arrAirportDiv, arrHintSpan;

  //Get the departure information
  findElement(locationsRow, TidyTag_TD, 1, TidyAttr_CLASS, "track-panel-departure", &depCell);
  findElement(depCell, TidyTag_DIV, 1, TidyAttr_CLASS, "track-panel-airport", &depAirportDiv);
  findElement(depAirportDiv, TidyTag_SPAN, 1, TidyAttr_CLASS, "hint", &depHintSpan);
  ctmbstr depLoc = 0;
  TidyAttr attr = tidyAttrGetById(depHintSpan, TidyAttr_TITLE);
  depLoc = tidyAttrValue(attr);
  a->flight_orig = (char *) malloc(depLoc?strlen(depLoc):2);
  strcpy(a->flight_orig, depLoc?depLoc:"?");

  //Get the arrival information
  findElement(locationsRow, TidyTag_TD, 1, TidyAttr_CLASS, "track-panel-arrival", &arrCell);
  findElement(arrCell, TidyTag_DIV, 1, TidyAttr_CLASS, "track-panel-airport", &arrAirportDiv);
  findElement(arrAirportDiv, TidyTag_SPAN, 1, TidyAttr_CLASS, "hint", &arrHintSpan);
  ctmbstr arrLoc = 0;
  attr = tidyAttrGetById(arrHintSpan, TidyAttr_TITLE);
  arrLoc = tidyAttrValue(attr);
  a->flight_dest = (char *) malloc(arrLoc?strlen(arrLoc):2);
  strcpy(a->flight_dest, arrLoc?arrLoc:"?");
}

/* Get the actual departure and predicted arrival times for the flight */
void getTimes(TidyDoc tdoc, TidyNode timesRow, struct aircraft *a)
{
  TidyNode depTimeCell = tidyGetChild(timesRow);
  TidyNode depTimeText = tidyGetChild(tidyGetChild(depTimeCell));
  while (tidyNodeGetName(depTimeText)) {
    depTimeText = tidyGetChild(depTimeText);
  }
  //Time and tz may all be wrapped in an <em> tag
  //TidyNode depTimeTz = tidyGetChild(tidyGetNext(depTimeText));
  a->dep_time = getElementText(tdoc, depTimeText);
  //TODO include tz

  TidyNode arrTimeCell = tidyGetNext(depTimeCell);
  TidyNode arrTimeText = tidyGetChild(tidyGetChild(arrTimeCell));
  //Check whether the time is wrapped in an <em> tag, if so go a level deeper
  while (tidyNodeGetName(arrTimeText)) {
    arrTimeText = tidyGetChild(arrTimeText);
  }
  //TidyNode arrTimeTz = tidyGetChild(tidyGetNext(arrTimeText));
  a->arr_time = getElementText(tdoc, arrTimeText);
  //TODO include tz
  //a->dep_time = "never";
  //a->arr_time = "never";
}

/* Get the scheduled departure and arrival times for the flight */
void getScheduledTimes(TidyDoc tdoc, TidyNode scheduledTimesRow, struct aircraft *a)
{
  TidyNode depTimeCell = tidyGetChild(scheduledTimesRow);
  TidyNode depTimeRow = tidyGetChild(tidyGetChild(tidyGetChild(depTimeCell)));
  TidyNode depTimeSubCell = tidyGetNext(tidyGetChild(depTimeRow));
  TidyNode depTimeText = tidyGetChild(depTimeSubCell);
  //TidyNode depTimeTz = tidyGetChild(tidyGetNext(depTimeText));
  a->sched_dep = getElementText(tdoc, depTimeText);

  TidyNode arrTimeCell = tidyGetNext(depTimeCell);
  TidyNode arrTimeRow = tidyGetChild(tidyGetChild(tidyGetChild(arrTimeCell)));
  TidyNode arrTimeSubCell = tidyGetNext(tidyGetChild(arrTimeRow));
  TidyNode arrTimeText = tidyGetChild(arrTimeSubCell);
  //TidyNode arrTimeTz = tidyGetChild(tidyGetNext(arrTimeText));
  a->sched_arr = getElementText(tdoc, arrTimeText);

  //TODO: set a->sched_dep and a->sched_arr
  //a->sched_dep = "never";
  //a->sched_arr = "never";
}

int getFlightInfo(struct aircraft *a)
{
  printf("Flight %s\n", a->flight);
  char *urlStart = "http://flightaware.com/live/flight/";
  char *url = (char*) malloc(strlen(a->flight) + strlen(urlStart));
  strcpy(url, urlStart);
  strcat(url, a->flight);
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
          getInfo(tdoc, a);
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

 
 
/* 
 // * gcc -Wall -I/usr/local/include flight_info.c -lcurl -ltidy
int main(int argc, char **argv )
{
  struct aircraft *a = malloc(sizeof(struct aircraft));
  strcpy(a->flight, "DAL2357");
  int err = getFlightInfo(a);
  printf("Origin: %s\n", a->flight_orig);
  printf("Destination: %s\n", a->flight_dest);
  printf("Actual/Predicted Departure Time: %s\n", a->dep_time);
  printf("Actual/Predicted Arrival Time: %s\n", a->arr_time);
  printf("Scheduled Departure Time: %s\n", a->sched_dep);
  printf("Scheduled Arrival Time: %s\n", a->sched_arr);
  return err;
}*/