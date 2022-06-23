TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

PROD_HOST += caMonitor
caMonitor_SRCS  += archiver.c
caMonitor_SRCS	+= caMonitor.c
caMonitor_LIBS	+= $(EPICS_BASE_HOST_LIBS)
caMonitor_LIBS += taos
include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

