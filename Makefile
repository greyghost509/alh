#*************************************************************************
# Copyright (c) 2002 The University of Chicago, as Operator of Argonne
# National Laboratory.
# Copyright (c) 2002 Deutches Elektronen-Synchrotron in der Helmholtz-
# Gemelnschaft (DESY).
# Copyright (c) 2002 Berliner Speicherring-Gesellschaft fuer Synchrotron-
# Strahlung mbH (BESSY).
# Copyright (c) 2002 Southeastern Universities Research Association, as
# Operator of Thomas Jefferson National Accelerator Facility.
# Copyright (c) 2002 The Regents of the University of California, as
# Operator of Los Alamos National Laboratory.
# This file is distributed subject to a Software License Agreement found
# in the file LICENSE that is included with this distribution. 
#*************************************************************************
#
# $Id$
#
TOP=../..
include $(TOP)/configure/CONFIG

#===========================
# Debugging options

ifeq ($(OS_CLASS), Darwin)
GCC_OPT_YES = -O2
endif

# For purify, EPICS base must not use READLINE 
#HOST_OPT=NO
#DEBUGCMD = purify -first-only -chain-length=40 -max_threads=256
#DEBUGCMD = purify -first-only -chain-length=40 -max_threads=256 \
#           -always-use-cache-dir -cache-dir=$(shell $(PERL) $(TOP)/config/fullPathName.pl .)
#DEBUGCMD = purify -first-only -chain-length=40 -max_threads=256 \
#           -always-use-cache-dir -cache-dir=$(shell $(PERL) $(TOOLS)/fullPathName.pl .)

#=============================

I_WANT_CDEV             = NO
I_WANT_GREETINGS        = NO
I_WANT_CMLOG		= NO

# These should really be done in <TOP>/config/RELEASE, but
# defining things here overrides these values.
#CMLOG=/opt/OPI/cmlog
##EPICS_BASE=/home/controls/epics/baseR3.13.1
#CDEV_DIR = /home/phoebus6/JBA/jlab/cdev1-7
#CDEVINCLUDE = $(CDEV_DIR)/include
#CDEVLIB = $(CDEV_DIR)/lib/$(HOST_ARCH)

ifeq ($(I_WANT_GREETINGS),YES)
alh_CFLAGS += -DIWantGreetings
alh_SRCS += productDescriptionShell.c
endif

ifeq ($(DEBUG), YES)
  USR_CCFLAGS   += -DDEBUG
endif

Linux_CPPFLAGS += -D_SVID_SOURCE -D_BSD_SOURCE
printer_CPPFLAGS += $($(OS_CLASS)_CPPFLAGS)
file_CPPFLAGS += $($(OS_CLASS)_CPPFLAGS)
alLog_CPPFLAGS += $($(OS_CLASS)_CPPFLAGS)

ifeq ($(I_WANT_CMLOG),YES)
  CMLOG_BIN		 = $(CMLOG)/bin/$(HOST_ARCH)
  alLog_CFLAGS	+= -I$(CMLOG)/include -DCMLOG
  file_CFLAGS   += -I$(CMLOG)/include -DCMLOG
  awAlh_CFLAGS	+= -DCMLOG -DCMLOG_CONFIG="\"cmlogrc.ALH\"" \
                   -DCMLOG_BROWSER="\"$(CMLOG_BIN)/cmlog -xrm '*geometry: 1400x500' -c\""
  alh_LIBS	+= cmlog
  cmlog_DIR	= $(CMLOG)/lib/$(HOST_ARCH)
  # Use following line if cmlog_DIR not in LD_LIBRARY_PATH
  USR_LDFLAGS_solaris =  -R$(cmlog_DIR)

  LINK.c = $(LINK.cc)
  CPLUSPLUS = CCC
endif

.PHONY:	libcmlog.a

# Some linux systems moved RPC related symbols to libtirpc
# Define TIRPC in configure/CONFIG_SITE in this case
ifeq ($(TIRPC),YES)
  USR_INCLUDES += -I/usr/include/tirpc
  SYS_PROD_LIBS_Linux += tirpc
endif

ifeq ($(I_WANT_CDEV),YES)

  CDEV_LIB      = $(CDEVLIB)
  CDEV_INC      = $(CDEVINCLUDE)

  #USR_CXXFLAGS      += -g
  #USR_CXXFLAGS      += -D__STDC__
  #USR_LDFLAGS      += -Wl,-E

  USR_CXXFLAGS      += -features=no%conststrings
  USR_CXXFLAGS      += -I$(CDEV_INC)

  PROD_LIBS_DEFAULT += caService cdev
  cdev_DIR = $(CDEV_LIB)
  caService_DIR = $(CDEV_LIB)

ifeq ($(ANSI), GCC)
  PROD_LIBS += EpicsCa ca
  EpicsCa_DIR = $(CDEV_LIB)
endif
  SYS_PROD_LIBS_DEFAULT += y l

  alh_SRCS         += alCaCdev.cc
else
  PROD_LIBS += ca

  alh_SRCS         += alCA.c
endif

USR_INCLUDES += -I../os/$(OS_CLASS) -I../os/default

#USR_INCLUDES += -I/usr/contrib/X11R5/include -I$(MOTIF_INC) -I$(X11_INC)
USR_INCLUDES += -I$(MOTIF_INC) -I$(X11_INC)

# baseR3.13.0.beta12 and later
awAlh_CFLAGS += -DALH_HELP_URL="\"http://www.aps.anl.gov/asd/controls/epics/EpicsDocumentation/ExtensionsManuals/AlarmHandler/ALHUserGuide/ALHUserGuide.html\""

USER_VPATH += ../os/$(OS_CLASS)
USER_VPATH += ../os/default

alh_SRCS += acknowledge.c
alh_SRCS += alAudio.c
alh_SRCS += alarm.c
alh_SRCS += alCaCommon.c
alh_SRCS += alConfig.c
alh_SRCS += alFilter.c
alh_SRCS += alLib.c
alh_SRCS += alLog.c
alh_SRCS += alView.c
alh_SRCS += alh.c
alh_SRCS += awAct.c
alh_SRCS += awAlh.c
alh_SRCS += awEdit.c
alh_SRCS += awView.c
alh_SRCS += axArea.c
alh_SRCS += axRunW.c
alh_SRCS += axSubW.c
alh_SRCS += browser.c
alh_SRCS += current.c
alh_SRCS += dialog.c
alh_SRCS += file.c
alh_SRCS += force.c
alh_SRCS += guidance.c
alh_SRCS += heartbeat.c
alh_SRCS += help.c
alh_SRCS += line.c
alh_SRCS += mask.c
alh_SRCS += process.c
alh_SRCS += property.c
alh_SRCS += scroll.c
alh_SRCS += showmask.c
alh_SRCS += sllLib.c
alh_SRCS += beepSevr.c
alh_SRCS += noAck.c

alh_DB_SRCS = alh_DB.c  

alh_printer_SRCS = printer.c

PROD_HOST_DEFAULT = alh alh_printer  alh_DB
PROD_HOST_WIN32 = alh

WIN32_RUNTIME=MD
USR_CFLAGS_WIN32 += -DWIN32 -D_WINDOWS
ifndef BORLAND
USR_LDFLAGS_WIN32 += /SUBSYSTEM:WINDOWS
endif

PROD_LIBS += Com
PROD_LIBS_DEFAULT += Xmu Xm Xt X11
PROD_LIBS_Linux +=  Xmu Xm Xt X11
PROD_LIBS_Darwin +=  Xmu Xm Xt X11

PROD_LIBS_WIN32 += $(EXCEED_XLIBS)
SYS_PROD_LIBS_WIN32 += winmm user32
USR_CFLAGS_WIN32 += $(EXCEED_CFLAGS)

SYS_PROD_LIBS_solaris += socket nsl

Xmu_DIR = $(X11_LIB)
#Xmu_DIR = /usr/contrib/X11R6/lib
Xm_DIR = $(MOTIF_LIB)
Xt_DIR = $(X11_LIB)
X11_DIR = $(X11_LIB)

RCS_WIN32 += alh.rc

include $(TOP)/configure/RULES

alh.res:../alh.ico
alh.res:../version.h

