#pragma once

#include <taos.h> 
#include <stdio.h>
#include <epicsStdlib.h>
#include <string.h>
#include <epicsMutex.h> 
#include <epicsEvent.h>
#include <epicsThread.h>
#include <cadef.h>
#include <epicsGetopt.h>
#include <tool_lib.h>
#include <syslog.h>
#include "fifo.h"
#include "taos_interface.h"





#define MAX_PV 2500
#define MAX_PV_NAME_LEN 40

typedef struct{
    char        value[20];
    chid        mychid;
    evid        myevid;
} MYNODE;



/*--------------
       全局变量，存储缓存指针、数据库连接指针、pv列表和一些统计信息
       -需要增加的全局状态信息可以放在这个里面
       -利用此结构体可以访问所有的PV
----------------*/
typedef struct{          
    long            data_rate;   /* the element count of the item return*/
    time_t          lastest_update_times;    /*pv time stamp */
    long            connected_pvs;     
    long            disconnected_pvs;
    long            callbacks_perminites;
    FIFO            *ring_buffer;
    TAOS            *taos;
    pv**            nodelist;
    int             nPv;
    epicsEventId    evt_newdata_in;
    TAOS_STMT       *stmt;
    void            *s3client;
} ARCHIVER;

ARCHIVER* Archiver;


typedef int16_t ARCHIVE_ERROR;
ARCHIVER *archive_initial();
ARCHIVE_ERROR archive_pv(evargs eha);

void archive_thread(ARCHIVER *parchiver);
ARCHIVE_ERROR start_archive_thread(ARCHIVER *archiver);

ARCHIVE_ERROR archiver_monitor_thread(ARCHIVER *archiver);

ARCHIVE_ERROR start_archiver_monitor(ARCHIVER *archiver);