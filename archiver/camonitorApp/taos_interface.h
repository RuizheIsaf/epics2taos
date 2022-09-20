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

typedef struct {
    int64_t     ts;
    float         val;
    char        *status;
    char        *serverity;
} ROWPV;

typedef struct {
    int64_t     ts;
    int         status;
} ROWST;

typedef struct {
    int64_t ts;
    int     callbackcounts;
    int     npvon;
    int     npvoff;
} ROWMPV;

int Pv2TD(TAOS * taos,ARCHIVE_ELEMENT data);
int PVStatus2TD(TAOS * taos, pv * data, int status);
int HB2TD(TAOS * taos, int callBackCounts, int nPVOnline, int nPVOffline);
TAOS* TaosConnect();