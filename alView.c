/*
 $Log$
 Revision 1.5  1998/05/12 18:22:42  evans
 Initial changes for WIN32.

 Revision 1.4  1997/09/12 19:30:14  jba
 Added test for treeSym.

 Revision 1.3  1995/10/20 16:50:02  jba
 Modified Action menus and Action windows
 Renamed ALARMCOMMAND to SEVRCOMMAND
 Added STATCOMMAND facility
 Added ALIAS facility
 Added ALARMCOUNTFILTER facility
 Make a few bug fixes.

 * Revision 1.2  1994/06/22  21:16:43  jba
 * Added cvs Log keyword
 *
 */

static char *sccsId = "@(#)alView.c	1.6\t9/20/93";

/*  alView.c
 *      Author: Janet Anderson
 *      Date:   12-20-90
 *
 *      Experimental Physics and Industrial Control System (EPICS)
 *
 *      Copyright 1991, the Regents of the University of California,
 *      and the University of Chicago Board of Governors.
 *
 *      This software was produced under  U.S. Government contracts:
 *      (W-7405-ENG-36) at the Los Alamos National Laboratory,
 *      and (W-31-109-ENG-38) at Argonne National Laboratory.
 *
 *      Initial development by:
 *              The Controls and Automation Group (AT-8)
 *              Ground Test Accelerator
 *              Accelerator Technology Division
 *              Los Alamos National Laboratory
 *
 *      Co-developed with
 *              The Controls and Computing Group
 *              Accelerator Systems Division
 *              Advanced Photon Source
 *              Argonne National Laboratory
 *
 * Modification Log:
 * -----------------
 * .02  02-16-93        jba     View routines for new user interface
 * .nn  mm-dd-yy        iii     Comment
 *      ...
 */
/* alView.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sllLib.h"
#include "alLib.h"
#include "ax.h"



/*
**************************************************************
	routines defined in alView.c
*************************************************************
*
*-----------------------------------------------------------------
*    routines related to the current configuration view
*-----------------------------------------------------------------
* 
int alViewAdjustGroupW(glink,viewFilter) Returns groupWindow viewCount
     GLINK *glink;                    for new glink
     int (viewFilter)();
* 
int alViewAdjustTreeW(glink, command, viewFilter) Uses command to adjust treeW view 
     GLINK *glink;                    at glink and return new viewCount
     int command;
     int (viewFilter)();
*
GCLINK *alViewNextTreeW(glink,plinkType)   Return next treeWindow item open 
     GLINK  *glink;                   for view
     int *plinkType;
*
GCLINK *
alViewNextGroupW(link,plinkType)       Returns next groupWindow item 
     GCLINK  *link;
     int *plinkType;
     int (viewFilter)();
*
GCLINK *
alViewNthTreeW(glinkStart,plinkType,n)     Returns Nth treeWindow item open
     GLINK        *glinkStart;        for view
     int          *plinkType;
     int           n;
*
GCLINK *
alViewNthGroupW(link,plinkType,n)      Returns Nth groupWindow item
     GLINK      *link;                
     int         *plinkType;
     int          n;
*
int alViewMaxSevrNTreeW(glinkStart,n)      treeWindow - Get max sevr in range
     GLINK       *glinkStart;         from glinkstart to Nth item following
     int          n;                  glinkstart
*
int alViewMaxSevrNGroupW(linkStart,n)  groupWindow - Get max sevr in range
     GCLINK       *linkStart;         from glinkstart to Nth item following
     int          n;                  glinkstart
*
 -------------
 |  PRIVATE  |
 -------------
*
static void treeView(glink, command, treeSym, viewFilter)
     GLINK *glink;
     int command;
     char *treeSym
     int (viewFilter)();

*/



#ifdef __STDC__

static void treeView( GLINK *glink, int command, char *treeSym, int (*viewFilter)());

#else

static void treeView();

#endif /*__STDC__*/


/***************************************************
  alViewAdjustGroupW
****************************************************/

int alViewAdjustGroupW(glink, viewFilter)
     GLINK *glink;
     int (*viewFilter)();
{
     SNODE *pt;
     int count=0;

     if (!glink) return(0);
     pt = sllFirst(&glink->subGroupList);
     while ( pt){
          if (glink->viewCount <= 1) 
               ((GCLINK *)pt)->viewCount = viewFilter((GCLINK *)pt);
          if( ((GCLINK *)pt)->viewCount ) count++;
          pt = sllNext(pt);
     }
     pt = sllFirst(&(glink->chanList));
     while (pt){
          ((GCLINK *)pt)->viewCount = viewFilter((GCLINK *)pt);
          if( ((GCLINK *)pt)->viewCount ) count++;
          pt = sllNext(pt);
     }
     return(count);
}

/***************************************************
  alViewAdjustTreeW
****************************************************/

int alViewAdjustTreeW(glink, command, viewFilter)
     GLINK *glink;
     int command;
     int (*viewFilter)();
{
     GLINK             *glinkTemp;
     int                holdViewCount, diffCount, count;
     char    treeSym[MAX_TREE_DEPTH+1];

     memset(treeSym,'\0',MAX_TREE_DEPTH);
     if (glink->pgroupData->treeSym)
          strcpy(treeSym,glink->pgroupData->treeSym);


     holdViewCount = glink->viewCount;
     treeView(glink, command, treeSym, viewFilter);
     diffCount = glink->viewCount - holdViewCount;

     glinkTemp=glink;
     count = glinkTemp->viewCount;
     while (glink){
          glinkTemp = glinkTemp->parent;
          if (glinkTemp == NULL ) break;
          glinkTemp->viewCount += diffCount;
          count = glinkTemp->viewCount;
     }
     return(count);
}

/***************************************************
  treeView
****************************************************/

static void treeView(glink, command, treeSym, viewFilter)
     GLINK *glink;
     int command;
     char  *treeSym;
     int (*viewFilter)();
{
     SNODE	*pt;
     int	 subcommand;
     int	 oldViewCount=0;
     int length;
     static char symMiddle={0x15};
     static char symContinue={0x19};
     static char symEnd={0x0E};
     static char symBlank={0x20};
     static char symNull={0x00};

     if (glink == NULL) return;

     if (command != NOCHANGE){
          oldViewCount = glink->viewCount;
          glink->viewCount = viewFilter((GCLINK *)glink);
     }

     length = strlen(treeSym);
     if (length >= MAX_TREE_DEPTH) return;

     if (glink->pgroupData->treeSym &&
         (length+1) > (int)strlen(glink->pgroupData->treeSym) )  {
          free(glink->pgroupData->treeSym);
          glink->pgroupData->treeSym = 0 ;
     }

     if (!glink->pgroupData->treeSym)
          glink->pgroupData->treeSym = (char *)calloc(1,length+1);

     /* find next view sibling */
     pt = sllNext(glink);
     while (pt){
          if (command == NOCHANGE){
               if (((GLINK *)pt)->viewCount>0) break;
          } else {
               if (viewFilter((GCLINK *)pt)) break;
          }
          pt = sllNext(pt);
     }

     if (length){
          if (pt) treeSym[length-1] = symMiddle;
          else treeSym[length-1] = symEnd;
          treeSym[length] = symNull;
          strcpy(glink->pgroupData->treeSym,treeSym);
     }

     if ( command == COLLAPSE) return;

     if ( command == NOCHANGE && glink->viewCount == 1) return;

     subcommand = command;
     if ( command == EXPANDCOLLAPSE1){
          if ( oldViewCount > 1)  return;
          subcommand=COLLAPSE;
     }

     if (length){
          if (pt) treeSym[length-1] = symContinue;
          else treeSym[length-1]=symBlank;
     }

     pt = sllFirst(&(glink->subGroupList));
     if (!pt) return;

     if (glink->viewCount) treeSym[length]=symBlank;

     while (pt){
          treeView((GLINK *)pt, subcommand, treeSym, viewFilter);
          if ( command != NOCHANGE) glink->viewCount += ((GLINK *)pt)->viewCount;
          pt = sllNext(pt);
     }

     treeSym[length]=symNull;
     return;
}

/***************************************************
  alViewNextTreeW
****************************************************/

GCLINK *alViewNextTreeW(glink,plinkType)
     GLINK  *glink;
     int *plinkType;
{
     GLINK  *glinkTemp;
     SNODE *pt;

     *plinkType = GROUP;

     /* try child */
     if (glink->viewCount >1){
          pt = sllFirst(&glink->subGroupList);
          while (pt){
               if (((GLINK *)pt)->viewCount) break;
               pt = sllNext(pt);
          }
          return((GCLINK *)pt);
     }

     /* try following siblings */
     pt = sllNext(glink);
     while (pt){
          if (((GLINK *)pt)->viewCount) break;
          pt = sllNext(pt);
     }
     if (pt) return((GCLINK *)pt);

     /* try parent siblings */
     glinkTemp = glink->parent;
     while (glinkTemp){
          pt = sllNext(glinkTemp);
          while (pt){
               if (((GLINK *)pt)->viewCount) break;
               pt = sllNext(pt);
          }
          if (pt) return((GCLINK *)pt);

          glinkTemp = glinkTemp->parent;
     }
     return(NULL);
}

/***************************************************
  alViewNextGroupW
****************************************************/

GCLINK *alViewNextGroupW(link,plinkType)
     GCLINK  *link;
     int *plinkType;
{
     SNODE *pt;
     GLINK  *parent;

     pt = sllNext((SNODE *)link);
     while(pt){
          if (((GCLINK *)pt)->viewCount) break;
          pt = sllNext(pt);
     }
     if (!pt && *plinkType == GROUP){
          parent = link->parent;
          pt = sllFirst(&(parent->chanList));
          *plinkType = CHANNEL;
          while(pt){
               if (((GCLINK *)pt)->viewCount) break;
               pt = sllNext(pt);
          }
     }
     return((GCLINK *)pt);
}

/***************************************************
  alViewNthTreeW
****************************************************/
GCLINK *alViewNthTreeW(glinkStart,plinkType,n)
     GLINK        *glinkStart;
     int          *plinkType;
     int           n;
{
     GLINK        *glink;
     int           count;
     SNODE       *pt;

     *plinkType = GROUP;

     if (!glinkStart) return(NULL);
     glink = glinkStart;
     if (!glink->viewCount) return(0);
     if (n > glink->viewCount) return(0);
     count = 0;

     while (count<n){
          if (count + glink->viewCount > n ){
               if (glink->viewCount) count++;
               glink = (GLINK *)sllFirst(&glink->subGroupList);
          }
          else {
               count += glink->viewCount;
               pt = sllNext(glink);
               while (!pt){
                    glink = glink->parent;
                    if (!glink) break;
                    pt = sllNext(glink);
               }
               glink = (GLINK *)pt;
               if (!glink) return(NULL);
          }
     }
     if (count != n) return(NULL);
     return((GCLINK *)glink);
}
/***************************************************
  alViewNthGroupW
****************************************************/
GCLINK *alViewNthGroupW(link,plinkType,n)
     GLINK      *link;
     int         *plinkType;
     int          n;
{

     SNODE       *pt;
     int          count= 0;

     *plinkType = 0;

     if (!link) return(0);
     pt = sllFirst(&link->subGroupList);
     while ( pt){
          if (count == n && ((GCLINK *)pt)->viewCount) break;
          if (((GCLINK *)pt)->viewCount) count++;
          pt = sllNext(pt);
     }
     if (pt) {
          *plinkType = GROUP;
          return((GCLINK *)pt);
     }
     pt = sllFirst(&(link->chanList));
     while (pt){
          if (count == n &&  ((GCLINK *)pt)->viewCount) break;
          if (((GCLINK *)pt)->viewCount) count++;
          pt = sllNext(pt);
     }
     if (pt) {
          *plinkType = CHANNEL;
     }
     return((GCLINK *)pt);
}
/***************************************************
  alViewMaxSevrNGroupW
****************************************************/
int alViewMaxSevrNGroupW(linkStart,n)
     GCLINK       *linkStart;
     int          n;
{
     GLINK       *glink;
     GLINK       *parent;
     CLINK       *clink=0;
     int          count= 0;
     int          sevr = 0;
     int          linkType;

     if (n <= 0 || !linkStart) return(0);

     /* determine linkType  of linkStart */
     parent = linkStart->parent;
     glink = (GLINK *)sllFirst(&parent->subGroupList);
     while ( glink){
          if ((GCLINK *)glink == linkStart ) break;
          glink = (GLINK *)sllNext(glink);
     }
     if (glink) {
          linkType = GROUP;
     } else {
          linkType = CHANNEL;
          clink = (CLINK *)sllFirst(&(parent->chanList));
          while (clink){
               if ((GCLINK *)clink == linkStart) break;
               clink = (CLINK *)sllNext(clink);
          }
     }
 
     /* determine maximum severity below linkStart */
     if (linkType == GROUP ){
          while ( glink){
               if (count == n) break;
               if (glink->viewCount) {
                    glink->pgroupData->curSevr = alHighestSeverity(glink->pgroupData->curSev);
                    sevr = Mmax(sevr,glink->pgroupData->curSevr);
                    if (sevr >= ALARM_NSEV-1) break;
                    count++;
               }
               glink = (GLINK *)sllNext(glink);
          }
          if (sevr >= ALARM_NSEV-1) return(sevr);
          if (glink) {
               return(sevr);
          }
          clink = (CLINK *)sllFirst(&(parent->chanList));
     }
     while (clink){
          if (count == n) break;
          if (clink->viewCount) {
               sevr = Mmax(sevr,clink->pchanData->curSevr);
               count++;
          }
          if (sevr >= ALARM_NSEV-1) break;
          clink = (CLINK *)sllNext(clink);
     }
     return(sevr);
}
/***************************************************
  alViewMaxSevrNTreeW
****************************************************/
int alViewMaxSevrNTreeW(glinkStart,n)
     GLINK       *glinkStart;
     int          n;
{
     GLINK       *glink;
     CLINK       *clink=0;
     SNODE       *pt;
     int          sevr=0;
     int          count=0;

     if (n <= 0 ) return(0);
     if (!glinkStart) return(-1);
     glink = glinkStart;
     if (!glink->viewCount) return(-1);

     while (count<n){
          if (count + glink->viewCount > n ){
               /*  max severity of channels within group */
               clink = (CLINK *)sllFirst(&(glink->chanList));
               while (clink){
                    sevr = Mmax(sevr,clink->pchanData->curSevr);
                    if (sevr >= ALARM_NSEV-1) break;
                    clink = (CLINK *)sllNext(clink);
               }
               if (sevr >= ALARM_NSEV-1) break;
               count++;
               glink = (GLINK *)sllFirst(&glink->subGroupList);
          }
          else {
               if (glink->viewCount)
                    glink->pgroupData->curSevr = alHighestSeverity(glink->pgroupData->curSev);
                    sevr = Mmax(sevr,glink->pgroupData->curSevr);
               if (sevr >= ALARM_NSEV-1) break;
               count += glink->viewCount;
               pt = sllNext(glink);
               while (!pt){
                    glink = glink->parent;
                    if (!glink) return(sevr);
                    pt = sllNext(glink);
               }
               glink = (GLINK *)pt;
          }
     }
     return(sevr);
}
