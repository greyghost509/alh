static char *sccsId = "@(#) $Id$";

/******************************************************************
  cdev access routines
******************************************************************/

#include <cdevSystem.h>
#include <cdevRequestObject.h>
#include <cdevDevice.h>
#include <cdevGroup.h>
#include <cdevErrCode.h>
#include <cdevClock.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

extern "C" {
#include "alarm.h"
#include "fdmgr.h"
#include "cadef.h"

extern void 	alChannelForceEvent	(void *, short);
extern void 	alGroupForceEvent	(void *, short);
extern void 	alLogConnection		(char *, char *);
extern void 	alNewAlarm		(int, int, char *, void *);
extern void 	alNewEvent		(int, int, int, char *, void *);
extern void 	alAckChan		(void *);
extern void 	registerCA		(void *, int, int);
}


//------------------------------
// Need the following external definitions
#define GROUP     		1
#define CHANNEL   		2

#define CA_SERVICE_NAME		"caService"

 
//------------------------------
// Define globals 
int toBeConnectedCount = 0;
unsigned long caDelay = 100;     /* ms */
extern XtIntervalId  caTimeoutId=(XtIntervalId)0;
static int	cdev_finished = 0;

//------------------------------
// Define statics 
static cdevSystem * systemAlh;
static char         buff[511];


//------------------------------
// forward declarations
static void alCaNewAlarmEvent   (int, void *, cdevRequestObject &, cdevData &);
static void alCaChannelForceEvent       (int, void *, cdevRequestObject &, cdevData
 &);
static void alCaGroupForceEvent         (int, void *, cdevRequestObject &, cdevData
 &);
static void SevrChangeConnEvent (int, void *, cdevRequestObject &, cdevData &);
static void alCaUpdate(XtPointer cd, XtIntervalId *id);

static void alCaChannelConnectionEvent(int,void *,cdevRequestObject &,cdevData &); 
static void alCaForcePVConnectionEvent(int,void *,cdevRequestObject &,cdevData &); 
static void alCaSevrPVConnectionEvent(int,void *,cdevRequestObject &,cdevData &);  
static void alCaChannelAccessRightsEvent(struct access_rights_handler_args args);
static void alCaForcePVAccessRightsEvent(struct access_rights_handler_args args);
static void alCaSevrPVAccessRightsEvent(struct access_rights_handler_args args);


// ==========================================================================
// ==========================================================================
/* cdevSignal
 *
 *	Stores a signal's device/attribute pair, as well as a request
 *	object for [device "get" attribute].  This request object
 *	is not used for sending requests, but rather for checking the
 *	validity of the device/message/attribute combination.  One is
 *	created for each call to alCaSearchName(), and destroyed in
 *	alCaClearChannel().
 */
class cdevSignal
{
private:
  char 			*dev;
  char			*attr;
  cdevRequestObject	*req;

public:
  cdevSignal	(char *name);
  ~cdevSignal	(void) 	{ if (dev) free (dev);  }

  char			*device		(void) 	{ return dev; }
  char			*attribute	(void) 	{ return attr; }
  cdevRequestObject	&request	(void) 	{ return *req; }
  int			connected	(void)
  {
    return (req->getState() == CDEV_STATE_CONNECTED);
  }
};

/* HP compiler won't inline this member because of iterative construct. */
cdevSignal::cdevSignal	(char *name)
{
  static cdevData	dummy;
  
  attr = dev = strdup (name);
  while (*attr && !isspace (*attr)) attr++;
  if (*attr) *attr++ = 0; /* terminate dev str and point attr at remainder */
  else attr = "VAL";
  
  sprintf (buff, "get %s", attr);
  req = cdevRequestObject::attachPtr (dev, buff);
  // need to send so that request object attempts to conenct
  req->sendNoBlock (0, &dummy);
}


// ==========================================================================
// ==========================================================================
/* cdevMonitor
 *
 *	Contains request object and callback associated with a
 *	device/message pair, along with most recent callback
 *	status and signal attributes (value, status, severity).
 *	Enables canceling callbacks --cdev needs the original
 *	callback object in order to turn off a monitor.
 */
class cdevMonitor
{
private:
  cdevCallback		*cb;
  cdevSignal		*signal;
  void			*data_;
  int			cb_status_;		// most recent callback status
  int			status_, severity_;	// signal status, severity
  
public:
  cdevMonitor	(cdevSignal *sig)
      : cb (0),
        signal (sig),
        data_ (0),
        cb_status_ (CDEV_NOTCONNECTED),
        status_ (0),
        severity_ (0)
  {
    // nothing to do here!
  }
  
  ~cdevMonitor	(void) { if (cb) delete cb; }

  // start
  //
  //	Sets up a monitorOn connection which will invoke the given
  //	function on change.  The user data argument is contained
  //	inside this object, which will be supplied as the client
  //	argument to the callback.  Inside the callback function,
  //	then, the user data will be accessible via the ::data()
  //	member of this object.
  int	start	(cdevCallbackFunction func, void *d, cdevData &context)
  {
    cdevRequestObject	*request;

    data_ = d;
    if (cb) delete cb;
    cb = new cdevCallback (func, this);
    sprintf (buff, "monitorOn %s", signal->attribute());
    request = cdevRequestObject::attachPtr (signal->device(), buff);
    request->setContext (context);

#ifdef DEBUG
    fprintf
      (stderr, "sendCallback: device = %s, message = %s\n",
       signal->device(), buff);
    fflush (stderr);
#endif

    return request->sendCallback (0, *cb);
  }


  // data
  //
  //	Returns the client data argument.
  void	*data	(void) { return data_; }


  // cb_status
  //
  //	Without an argument, returns a CDEV_XXX value which corresponds
  //	to the value from the most recent callback.  Initial value is
  //	CDEV_NOT_CONNECTED.  With and argument, sets the object's state.
  int	cb_status	(void) 	{ return cb_status_; }
  void	cb_status	(int s)	{ cb_status_ = s; }


  // status/severity
  //
  //	Without arguments, these routines return the most recently
  //	set value for the appropriate property.  With, they set
  //	the associated attribute locally (not in the control-system!).
  int	status		(void) 	{ return status_; }
  int	severity	(void) 	{ return severity_; }
  void	status		(int s) { status_ = s; }
  void	severity	(int s) { severity_ = s; }

  
  // stop
  //
  //	Turn off the monitor.
  int	stop	(void)
  {
    cdevRequestObject	*request;
    
    sprintf (buff, "monitorOff %s", signal->attribute());
    request = cdevRequestObject::attachPtr (signal->device(), buff);

    // Bad business here: can't turn off specific callback.  The way
    // cdev works presently, in order to turn off a specific monitor
    // you have to pass the same cdevCallback object with the same
    // device/attribute and the message "monitorOff".  This gives
    // rise to many potentially nasty errors, however, because the
    // cdev service provides no general way for clients to know that
    // the callback has finished.
    //
    // Workaround:
    //
    //	assumption: 	only one monitorOn request for every device/attribute
    //             	pair.
    //	method:		send "monitorOff" with null callback in order to
    //			cancel *all* monitors for the device/attribute pair.

    return request->sendNoBlock (0, 0);
    //    return request->sendCallback (0, *cb);
  }
};


extern "C" {
/******************************************************************************
 alCaPend
******************************************************************************/
void     alCaPend (double sec)
{
  static cdevClock      timer;
 
#ifdef DEBUG
  fprintf (stderr, "cdevSystem::poll() for %f seconds\n", sec);
  fflush (stderr);
#endif
  timer.schedule (0, sec);
  while (toBeConnectedCount>0 && !timer.expired()) systemAlh->poll();
}

/******************************************************************************
 alCaIsConnected
******************************************************************************/
short alCaIsConnected(chid chid)
{
  cdevSignal	*signal = (cdevSignal *)chid;

  if (!signal) return FALSE;
  if (signal->connected()) return TRUE;
  else return FALSE;
}
 
/******************************************************************************
  alCaFlushIo
  ******************************************************************************/
void	alCaFlushIo(char *)
{
#ifdef DEBUG
  fprintf (stderr, "cdevSystem::flush(); cdevSystem::poll()\n");
  fflush (stderr);
#endif  
  systemAlh->flush();
}

/******************************************************************************
  initializes channel access
  register channel access file descriptor
  ******************************************************************************/
void	alCaInit()
{
  systemAlh = &cdevSystem::defaultSystem();
  systemAlh->setThreshold (CDEV_SEVERITY_ERROR);
  systemAlh->addFdChangedCallback (alFdmgrAddCdev, 0);
  caTimeoutId = XtAppAddTimeOut(appContext,caDelay,alCaUpdate,NULL);
}

/******************************************************************************
  perform CA handling when events exist
  ******************************************************************************/
void	alCaPoll()
{
  if (systemAlh)
  {
#ifdef DEBUG
    fprintf (stderr, "cdevSystem::flush(); cdevSystem::poll()\n");
    fflush (stderr);
#endif  
    /* systemAlh->flush(); shouldn't be needed*/
    systemAlh->poll();
  }
}

/******************************************************************************
  cancel registration of ca fds, cancel timeout, and exit ca task
  ******************************************************************************/
void	alCaStop()
{
  /* cancel timeout */
  if (caTimeoutId) {
    XtRemoveTimeOut(caTimeoutId);
    caTimeoutId = (XtIntervalId)0;
  }
  cdev_finished = 1;
}

/******************************************************************************
  add alarm event handler for a channel
  ******************************************************************************/
void	alCaSearchName(char *name, chid *pchid)
{
  cdevSignal	*signal = new cdevSignal (name);
  toBeConnectedCount++;
  
  if (signal->request().getState() == CDEV_INVALID)
    alLogConnection (name, "alCdevSearchName: request object is invalid");
  *pchid = (chid)signal;
}

/******************************************************************************
  clear a channel chid
  ******************************************************************************/
void	alCaClearChannel (chid *pchid)
{
  if (*pchid) delete (cdevSignal *)*pchid;
  *pchid = NULL;
}

/******************************************************************************
    clear event
    ******************************************************************************/
void	alCaClearEvent	(evid *pevid)
{
  cdevMonitor	*monitor = (cdevMonitor *)pevid;

  if (monitor)
  {
    if (monitor->stop() != CDEV_SUCCESS)
      alLogConnection(ca_name((*pevid)->chan), "alCaClearEvent: error cancelling monitor");
    delete monitor;
  }
  *pevid = NULL;
}

/******************************************************************************
    add New Alarm Event
    ******************************************************************************/
void	alCaAddEvent	(chid 	chid,
                         evid 	*pevid,
                         void 	*clink)
{
  cdevSignal	*signal = (cdevSignal *)chid;
  cdevMonitor	*monitor;
  cdevData 	ctx;
  int		alarm = 0;
  
  if (!signal) return;

  // now, if our signal belongs to the channel access service, we
  // need to use a different context.
  if (strcmp (signal->request().service().name(), CA_SERVICE_NAME) == 0)
  {
    ctx.insert ("value", 1);
    ctx.insert ("severity", 2);
    ctx.insert ("status", 2);
  }
  else
  {
    ctx.insert ("value", 3);
    ctx.insert ("severity", 1);
    ctx.insert ("status", 1);
  }
  
  monitor = new cdevMonitor (signal);
  alarm = (monitor->start (alCaNewAlarmEvent, clink, ctx) != CDEV_SUCCESS);
  //  if (!signal->connected() || alarm)
  if (alarm)
  {
#ifdef DEBUG
    if (!signal->connected())
      fprintf (stderr, "%s.%s: not connected\n",
               signal->device(), signal->attribute());
    if (alarm)
      fprintf (stderr, "%s.%s: error initiating monitor\n",
               signal->device(), signal->attribute());
    fflush (stderr);
#endif
    alNewAlarm (COMM_ALARM, INVALID_ALARM, "0", clink);
  }

  *pevid = (evid)monitor;
}


/******************************************************************************
  add change connection event for sevrPV
  ******************************************************************************/
void	alCaAddSevrPVEvent	(chid, void *)
{
  // this doesn't apply to cdev device/attributes --severity is
  // retrieved via tag in callback object.
}

/******************************************************************************
  alCaAddForcePVEvent
  ******************************************************************************/
void    alCaAddForcePVEvent	(chid 	chid,
                                 char 	*,
                                 void 	*link,
                                 evid 	*pevid,
                                 int 	type)
{
  cdevSignal		*signal = (cdevSignal *)chid;
  cdevMonitor		*monitor;
  cdevData 		ctx;
  cdevCallbackFunction	func;

  if (!signal)
  {
    alLogConnection("alCaAddForcePVEvent:"," null cdevSignal pointer");
    return;
  }

  monitor = new cdevMonitor (signal);
  sprintf (buff, "%s %s", signal->device(), signal->attribute());

  if (type == GROUP)
    func = alCaGroupForceEvent;
  else if (type == CHANNEL)
    func = alCaChannelForceEvent;
  else alLogConnection (buff, "alCaAddForcePVEvent: Invalid type error");

  ctx.insert ("value", 0);
  
  if (monitor->start (func, link, ctx) != CDEV_SUCCESS)
    alLogConnection(buff, "alCaAddForcePVEvent: error starting monitor");
  
  *pevid = (evid)monitor;
}

/******************************************************************************
  alCaPutGblAck
  ******************************************************************************/
void	alCaPutGblAck (chid chid, short *psevr)
{
  cdevSignal	*signal = (cdevSignal *)chid;
  cdevData 	out;
  
  if (!signal)
  {
    alLogConnection("alCaPutGblAck"," null cdevSignal pointer");
    return;
  }

  sprintf (buff, "ack %s", signal->attribute());
  out.insert ("value", *psevr);
  if (signal->request().device().sendNoBlock (buff, out, 0) != CDEV_SUCCESS)
  {
    sprintf (buff, "%s ack %s", signal->device(), signal->attribute());
    alLogConnection (buff, "alCaPutGblAck: error sending acknowledgement");
  }
}

/******************************************************************************
  alCaPutSev
  ******************************************************************************/
void	alCaPutSevrValue (chid chid, short *psevr)
{
  cdevSignal		*signal = (cdevSignal *)chid;
  cdevRequestObject	*request;
  cdevData 		out;
  
  if (!signal)
  {
    alLogConnection("alCaPutSevrValue","null cdevSignal pointer");
    return;
  }

  sprintf (buff, "set %s", signal->attribute());
  request = cdevRequestObject::attachPtr (signal->device(), buff);
  out.insert ("value", *psevr);
  if (request->sendNoBlock (out, 0) != CDEV_SUCCESS)
  {
    sprintf (buff, "%s set %s", signal->device(), signal->attribute());
    alLogConnection (buff, "alCaPutSevrValue: error writing severity");
  }
}


/******************************************************************************
 *  Close connection event
******************************************************************************/
static void closeConnectionEvent (int, void *, cdevRequestObject &, cdevData &)
{
  //  Dont know how to close cdev connection event
}

} // end extern "C"
 
/******************************************************************************
    alCaNewAlarmEvent get channel sevr, status, value
    then invoke alNewAlarm  & update value of SevrPVName
******************************************************************************/
static void alCaNewAlarmEvent (int 			status,
                           void 		*arg,
                           cdevRequestObject 	&req,
                           cdevData		&result)
{
  cdevMonitor	*monitor = (cdevMonitor *)arg;
  
  if (cdev_finished) return;
  
  sprintf (buff, "%s %s", req.device().name(), req.message());

#ifdef DEBUG
  fprintf (stderr, "callback: %s\n", buff);
  fflush (stderr);
#endif  
  switch (status)
  {
      case CDEV_RECONNECTED:
        alLogConnection (buff, " Reconnected (Channel  PVname)");

      case CDEV_SUCCESS:
      {
        char	valstr[MAX_STRING_SIZE+1];
        char	*pstr;
        double	value;
        int 	sevr;
        int 	stat;
        int	changed = 0;

        // if this is the first event, let's turn off any associated alarms
        if (monitor->cb_status() == CDEV_NOTCONNECTED)
          toBeConnectedCount--;
          // alAckChan (monitor->data());	// gee, I hope this works

        // hey, cdevData can do the value --> string conversion for us!
        if (result.get ("value", valstr, MAX_STRING_SIZE) == CDEV_SUCCESS)
          pstr = valstr;
        else pstr = "Unknown";

        if (result.get ("severity", &sevr) == CDEV_SUCCESS)
        {
          changed = (changed || (monitor->severity() != sevr));
          monitor->severity (sevr);
        }

        if (result.get ("status", &stat) == CDEV_SUCCESS)
        {
          changed = (changed || (monitor->status() != stat));
          monitor->status (stat);
        }

        // only call alNewEvent if either the status or severity
        // has changed
        if (changed)
          alNewEvent
            (monitor->status(), monitor->severity(), 0, pstr, monitor->data());
      }
      break;
        
      case CDEV_NOACCESS:
      {
        alNewAlarm (READ_ACCESS_ALARM, INVALID_ALARM, "0", monitor->data());
        alLogConnection (buff, "No read access (Channel  PVname)");
      }
      break;
        
      case CDEV_DISCONNECTED:
      {
        alNewAlarm (COMM_ALARM, INVALID_ALARM, "0", monitor->data());
        alLogConnection (buff, "Not Connected (Channel  PVname)");
      }
      break;
        
      default:
      {
        alNewAlarm (COMM_ALARM, INVALID_ALARM, "0", monitor->data());
        alLogConnection (buff, "Error while monitoring (Channel  PVname)");
      }
      break;
  }

  monitor->cb_status (status);
}

/******************************************************************************
 *  Group Force Event Callback
******************************************************************************/
static void alCaGroupForceEvent (int 		status,
                             void 		*arg,
                             cdevRequestObject 	&req,
                             cdevData		&result)
{
  cdevMonitor	*monitor = (cdevMonitor *)arg;
  
  if (cdev_finished) return;
  
  sprintf
    (buff, "%s--(%s %s)",
     (char *)monitor->data(), req.device().name(), req.message());

  switch(status)
  {
      case CDEV_RECONNECTED:
        alLogConnection(buff, "Reconnected   (Force Gp PVName)");
        
      case CDEV_SUCCESS:
        short value;

        toBeConnectedCount--;
        result.get ("value", &value);
        alGroupForceEvent (monitor->data(), value);
        break;
        
      case CDEV_NOACCESS:
        alLogConnection(buff, "No read access (Force Gp PVname)");
        break;
        
      case CDEV_DISCONNECTED:
        alLogConnection(buff, "Not Connected (Force Gp PVName)");
        break;
        
      default:
        alLogConnection(buff, "Error while monitoring (Force Gp PVName)");
        break;
  }

  monitor->cb_status (status);
}
 
/******************************************************************************
 *  Channel Force Event Callback
******************************************************************************/
static void alCaChannelForceEvent (int 			status,
                               void 			*arg,
                               cdevRequestObject 	&req,
                               cdevData			&result)
{
  cdevMonitor	*monitor = (cdevMonitor *)arg;
  
  if (cdev_finished) return;
  
  sprintf
    (buff, "%s--(%s %s)",
     (char *)monitor->data(), req.device().name(), req.message());
  
  switch(status)
  {
      case CDEV_RECONNECTED:
        alLogConnection(buff, "Reconnected   (Force Ch PVName)");
        
      case CDEV_SUCCESS:
        short value;

        toBeConnectedCount--;
        result.get ("value", &value);
        alChannelForceEvent (monitor->data(), value);
        break;
        
      case CDEV_NOACCESS:
        alLogConnection(buff, "No read access (Force Ch PVname)");
        break;
        
      case CDEV_DISCONNECTED:
        alLogConnection(buff, "Not Connected (Force Ch PVName)");
        break;
        
      default:
        alLogConnection(buff, "Error while monitoring (Force Ch PVName)");
        break;
  }

  monitor->cb_status (status);
}
 

/******************************************************************************
 alFdmgrAddCdev
******************************************************************************/
static void alFdmgrAddCdev (int fd, int opened, void *)
{
  registerCA (pfdctx, fd, opened);
}

/******************************************************************************
 alCaUpdate
******************************************************************************/
static void     alCaUpdate(XtPointer cd, XtIntervalId *id)
{
#ifdef DEBUG
  fprintf (stderr, "cdevSystem::poll()\n");
  fflush (stderr);
#endif
  systemAlh->poll();
  alUpdateAreas();
  caTimeoutId = XtAppAddTimeOut(appContext,caDelay,alCaUpdate,NULL);
}
