/* line.c */

/************************DESCRIPTION***********************************
  Routines for alloc, init, and update of a displayed line
**********************************************************************/

static char *sccsId = "@(#) $Id$";

#include <stdio.h>
#include <stdlib.h>

#include "alarm.h"

#include "alh.h"
#include "alLib.h"
#include "sllLib.h"
#include "axSubW.h"
#include "line.h"
#include "axArea.h"
#include "ax.h"

/* global variables */
extern char * alarmSeverityString[];
extern char * alarmStatusString[];

static char buff[LINEMESSAGE_SIZE];

/********************************************************************* 
 *  This function returns a string with 5 characters which shows
 *  the group mask.  The input to this function is the group
 *  mask array.
 *********************************************************************/
void awGetMaskString(short mask[ALARM_NMASK],char *s)
{
	strcpy(s,"-----");
	if (mask[ALARMCANCEL] > 0) *s = 'C';
	if (mask[ALARMDISABLE] > 0) *(s+1) = 'D';
	if (mask[ALARMACK] > 0) *(s+2) = 'A';
	if (mask[ALARMACKT] > 0) *(s+3) = 'T';
	if (mask[ALARMLOG] > 0) *(s+4) = 'L';
	*(s+5) = '\0';
}

/*********************************************************************
 *  This function allocates space for new line  
 *********************************************************************/
struct anyLine *awAllocLine()
{
	struct anyLine *pLine;
	pLine = (struct anyLine *)calloc(1,sizeof(struct anyLine));
	return(pLine);
}

/********************************************************************* 
 *  This function updates the channel line mask , status & severity
 *********************************************************************/
void awUpdateChanLine(struct anyLine *chanLine)
{
	struct chanData *cdata;
	CLINK *clink;

	clink = (CLINK *)chanLine->link;
	if (!clink) return;
	cdata = clink->pchanData;
	chanLine->unackSevr = cdata->unackSevr;
	chanLine->curSevr = cdata->curSevr;
	chanLine->unackStat = cdata->unackStat;
	chanLine->curStat = cdata->curStat;
	if (cdata->curMask.Disable == MASK_ON)
		chanLine->curSevr = 0;
	alGetMaskString(cdata->curMask,buff);
	sprintf(chanLine->mask,"<%s>",buff);

	strcpy(chanLine->message," ");
	if (cdata->curMask.Disable ) return;

	if (cdata->curSevr == 0 && cdata->unackSevr == 0) return;

	sprintf(chanLine->message,"<%s,%s>",
	    alarmStatusString[cdata->curStat],
	    alarmSeverityString[cdata->curSevr]);

	if (cdata->unackSevr == 0) return;

	sprintf(buff,",<%s,%s>",
	    alarmStatusString[cdata->unackStat],
	    alarmSeverityString[cdata->unackSevr]);
	strcat(chanLine->message,buff);
}

/*************************************************************************** 
 *  This function updates the group line mask , status & severity, summary
 ***************************************************************************/
void awUpdateGroupLine(struct anyLine *groupLine)
{
	struct groupData *gdata;
	GLINK *glink;
	int i;

	glink = (GLINK *)groupLine->link;
	if (!glink) return;
	gdata = glink->pgroupData;
	awGetMaskString(gdata->mask,buff);
	sprintf(groupLine->mask,"<%s>",buff);
	for (i=0;i<ALARM_NSEV;i++)
		groupLine->curSev[i] = gdata->curSev[i];
	groupLine->unackSevr = alHighestSeverity(gdata->unackSev);
	groupLine->curSevr = alHighestSeverity(gdata->curSev);

	strcpy(groupLine->message," ");
	if (groupLine->unackSevr == 0 && alHighestSeverity(gdata->curSev) == 0)
		return;

	sprintf(groupLine->message,"(%d,%d,%d,%d)",
	    gdata->curSev[INVALID_ALARM],
	    gdata->curSev[MAJOR_ALARM],gdata->curSev[MINOR_ALARM],
	    gdata->curSev[NO_ALARM]);
}

/***************************************************
  Initializes all line fields
****************************************************/
void initLine(struct anyLine *line)
{
	line->link = NULL;
	line->cosCallback = NULL;
	line->linkType = 0;
	line->unackSevr = 0;
	line->curSevr = 0;
	line->pname = 0;
	line->alias = 0;
}

/***************************************************
 Initializes all subWindow lines
****************************************************/
void initializeLines(SNODE *lines)
{
	struct anyLine *line;

	line = (struct anyLine *)sllFirst(lines);
	while (line){
		initLine(line);
		line = (struct anyLine *)sllNext(line);
	}

}

