TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

PROD_HOST += caMonitor
caMonitor_SRCS  += s3.cpp
#caMonitor_SRCS  += s3crtWrapper.cpp
caMonitor_SRCS  += archiver.c
caMonitor_SRCS	+= caMonitor.c
caMonitor_SRCS	+= tool_lib.c
caMonitor_SRCS	+= fifo.c
caMonitor_SRCS	+= taos_interface.c
caMonitor_SRCS  += loadConfig.c

caMonitor_LIBS	+= $(EPICS_BASE_HOST_LIBS)
caMonitor_LIBS  += taos
caMonitor_LIBS  += aws-cpp-sdk-core
caMonitor_LIBS  += aws-cpp-sdk-s3
OPT_CFLAGS_YES += -g
OPT_CXXFLAGS_YES += -std=c++11
OPT_CXXFLAGS_YES += -g
OPT_CXXFLAGS_YES += -D_GLIBCXX_USE_CXX11_ABI=0

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

