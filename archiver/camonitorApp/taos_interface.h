#pragma once

#include <taos.h> 
#include <stdio.h>
#include <epicsStdlib.h>
#include <string.h>
#include <epicsMutex.h> 
#include <epicsThread.h>
#include <cadef.h>
#include <epicsGetopt.h>
#include <tool_lib.h>
#include <syslog.h>
#include "archiver.h"


int Pv2TD(TAOS * taos,ARCHIVE_ELEMENT data);
int PVStatus2TD(TAOS * taos, pv * data);
TAOS* TaosConnect(char* host, char* user,char* passwd);