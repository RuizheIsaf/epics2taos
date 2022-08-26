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



<<<<<<< HEAD
#define MAX_DATA_LENGTH 128
=======
#define MAX_DATA_LENGTH 64
>>>>>>> e5b405315f6345677befccbd094ff17da64e2b59
typedef struct{
    char		    pvname[40];
    long            type;   /* type of pv */ 
    long            count;  /* the element count of the item return*/
    char            data[MAX_DATA_LENGTH];         /* value of the pv */
} ARCHIVE_ELEMENT;


typedef struct 
{
    int max_size;
    int write_position;
    int read_position;
    ARCHIVE_ELEMENT *buff;   
    epicsMutexId fifoLock;     
    epicsMutexId readLock;   
}FIFO;


#define FIFO_OK 1
#define FIFO_ERROR 0
#define FIFO_EMPTY 2
#define FIFO_OVERFLOW 3
#define BUFF_LENGTH 1000

//fifo_error fifoReadBulk(FIFO *fifo, int read_size, ARCHIVE_ELEMENT *readbuff);

typedef int16_t fifo_error;


fifo_error fifoInitial(FIFO * fifo,int fifo_size);
fifo_error fifoWrite(FIFO *fifo, ARCHIVE_ELEMENT data);
fifo_error fifoRead(FIFO *fifo, ARCHIVE_ELEMENT *data);

