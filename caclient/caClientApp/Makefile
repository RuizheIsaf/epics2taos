TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

PROD_HOST += caExample
caExample_SRCS	+= caExample.c
caExample_LIBS	+= $(EPICS_BASE_HOST_LIBS)

PROD_HOST += caMonitor
caMonitor_SRCS	+= caMonitor.c
caMonitor_SRCS	+= tool_lib.c
OPT_CFLAGS_YES += -g
OPT_CXXFLAGS_YES += -g

caMonitor_LIBS	+= $(EPICS_BASE_HOST_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

