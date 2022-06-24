#ifndef ARCHIVER_DEF
#define ARCHIVER_DEF


#include <taos.h> 
#include <stdio.h>
#include <epicsStdlib.h>
#include <string.h>
#include <epicsMutex.h> 
#include <epicsThread.h>
#include <cadef.h>
#include <epicsGetopt.h>
#include <tool_lib.h>



#define MAX_DATA_LENGTH 64
#define MAX_PV 1000
#define MAX_PV_NAME_LEN 40

typedef struct{
    char        value[20];
    chid        mychid;
    evid        myevid;
} MYNODE;

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
    pv              *nodelist;
} ARCHIVER;

ARCHIVER* Archiver;

#define FIFO_OK 1
#define FIFO_ERROR 0
#define FIFO_EMPTY 2
#define FIFO_OVERFLOW 3
#define BUFF_LENGTH 1000

typedef int16_t fifo_error;


fifo_error fifoInitial(FIFO * fifo,int fifo_size);
fifo_error fifoWrite(FIFO *fifo, ARCHIVE_ELEMENT data);
fifo_error fifoRead(FIFO *fifo, ARCHIVE_ELEMENT *data);
//fifo_error fifoReadBulk(FIFO *fifo, int read_size, ARCHIVE_ELEMENT *readbuff);

typedef int16_t ARCHIVE_ERROR;
ARCHIVER *archive_initial();
ARCHIVE_ERROR archive_pv(evargs eha);
void archive_thread(ARCHIVER *parchiver);
ARCHIVE_ERROR start_archive_thread(ARCHIVER *archiver);

#endif

