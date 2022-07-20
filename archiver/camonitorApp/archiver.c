
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <alarm.h>
#include <epicsString.h>
#include <cantProceed.h>

#include "archiver.h"
#include "time.h"



ARCHIVER*  archive_initial()
{
    ARCHIVER * archiver = (ARCHIVER *)callocMustSucceed(1, sizeof(ARCHIVER), "archiver");
    archiver->ring_buffer = (FIFO*) callocMustSucceed(1, sizeof(FIFO), "archiver");
    archiver->ring_buffer->buff = (ARCHIVE_ELEMENT*)callocMustSucceed(BUFF_LENGTH, sizeof(ARCHIVE_ELEMENT), "archiver");
    printf("fifo initial\n");
    fifoInitial(archiver->ring_buffer,BUFF_LENGTH);    //初始化缓存
    printf("fifo initial\n");

     //taos connect begin   by  ruizhe
    const char* host = "127.0.0.1";
    const char* user = "root";
    const char* passwd = "taosdata";
    archiver->taos= TaosConnect(host, user, passwd);
    return archiver;
}



/*----------------------------
    注意： caMonotor模板是错的。不能把eha.dbr直接转成char*（调试时看每子节的数据时可以用）！
    eha.dbr是一个结构体指针，根据eha.type的不同为不同的类型。
    要用如下的方法来操作。
-----------------------------*/
ARCHIVE_ERROR archive_pv(evargs eha)
{
    ARCHIVE_ELEMENT newdata;
    strcpy(&newdata.pvname,ca_name(eha.chid));
    newdata.type = eha.type;
    newdata.count = eha.count;


    memcpy(&newdata.data,eha.dbr,dbr_size_n(eha.type,eha.count));

   
    if(fifoWrite(Archiver->ring_buffer, newdata)==FIFO_OK)
    {
        //epicsMutexUnlock(Archiver->ring_buffer->readLock);
        /*-----------
        EPICS的互斥锁只能在同一个线程里成对使用。不能一个线程锁后在另一线程里解锁。上面这么写是错的
        -------------*/  
    } 
    else
    {
        printf("fifo write error!\n");
    }  
}


void archive_thread(ARCHIVER *parchiver)
{ 
    //ARCHIVER *archiver = parchiver;
    ARCHIVE_ELEMENT data;
    
    while (true)
    {
        //epicsMutexMustLock(Archiver->ring_buffer->readLock);
        if(fifoRead(Archiver->ring_buffer, &data)!= FIFO_EMPTY)
        {    
            #ifdef DEBUG
            printf("-----------------------\n");
            printf("archive thread called\n");
            printf("pvname: %s\n",data.pvname);
            //printf("testype: %s\n",data.type);
            printf("new value is %s\n",val2str (data.data, data.type,0));
            #endif         
        /*-------------这里增加写入TDengine的代码----------------
            //result = taos_query(Archiver->taos,str);
            //char* errstr = taos_errstr(result);
            //printf("query sql: %s \n query result: %s \n", str, errstr);
            //taos_free_result(result);

            要求：1. 数据库连接中断时可以自动重新连接
                 2. 把时间戳、警报状态也一起存储.(具体获得方法展开dbr2str里查看）    
        //----------------------------------------------------*/
             Pv2TD(Archiver->taos, data);           
        }
        else
        {
            //printf("fifo read error!\n");
        }
    }   
}

ARCHIVE_ERROR start_archive_thread(ARCHIVER *archiver)
{
    printf("start_archive_thread called\n");
    if (epicsThreadCreate("ArchiverTask", epicsThreadPriorityHigh,
		10000, (EPICSTHREADFUNC*)archive_thread, (void *)archiver)
		== (epicsThreadId) 0) {
		printf ("ArchiverTask spawn error\n");
        syslog(LOG_USER|LOG_INFO,"ArchiverTask spawn error\n"); 
		return -1;
	}
}



ARCHIVE_ERROR archiver_monitor_thread(ARCHIVER *archiver)
{
    pv * pvlisthead;
    int npv = archiver->nPv;
    long long callBackCounts=0;
    //int callBackCounts=0;
    int nPvOn = 0;
    int nPvOff = 0;
    pvlisthead = archiver->nodelist;
    
    

    //printf("pvlisthead in archiver_monitor_thread = %p\n\n\n\n",  pvlisthead);
    int i;

    while (true)
    {
        //*************************************************************************
        //将数据采集进程的状态写入数据库。。上层软件可以可以根据这个统计信息了解数据采集进程的状态。
        //这个也作为采集系统的心跳。。如果心跳数据不正常。。上层软件可以通过管理程序重启采集进程
        //**************************************************************************

        for (i = 0; i < npv; i++)
        {
            //printf("pvlisthead[%d] in archiver_monitor_thread = %p\n\n", i, pvlisthead[i]);
            callBackCounts = pvlisthead[i].callbackCounts;  //这就是所有pv总的回调次数。。利用类似的机制可以统计数据采集整体状态
            if(pvlisthead[i].isConnected == 1) {
                nPvOn++;
            } else {
                nPvOff++;
            }
        } 
        HB2TD(Archiver->taos, callBackCounts, nPvOn, nPvOff);
        callBackCounts = 0;  
        nPvOn = 0;
        nPvOff = 0;
        sleep(10); //等待10秒
    }
    
}

ARCHIVE_ERROR start_archiver_monitor(ARCHIVER *archiver)
{
        printf("start_archiver_monitor_thread called\n");
    if (epicsThreadCreate("ArchiverMonitorTask", epicsThreadPriorityHigh,
		1, (EPICSTHREADFUNC*)archiver_monitor_thread, (void *)archiver)
		== (epicsThreadId) 0) {
		printf ("ArchiverTask spawn error\n");
        syslog(LOG_USER|LOG_INFO,"ArchiverMonitorTask spawn error\n"); 
		return -1;
	}
}

/*-------------------
    环形缓存（Ring Buffer）
    好处：1.不需反复申请和注销内存
         2.速度快
         3.消费线程卡死时，不会导致缓存空间无限增大

    BUG：PV连接非常多时，偶尔会有一些PV被写入两次。可能是camonitor机制本身的问题，也可能是缓存一致性的问题。
         似乎不会丢失数据

    待改进：
         可以用EPICS的时间机制代替第81行的判断。
          
---------------------*/



