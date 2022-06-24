
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <alarm.h>
#include <epicsString.h>
#include <cantProceed.h>

#include "archiver.h"


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

    taos_options(TSDB_OPTION_TIMEZONE, "GMT-8");
    archiver->taos = taos_connect(host, user, passwd, "", 0);
    if (archiver->taos == NULL) {
      printf("\033[31mfailed to connect to db, reason:%s\033[0m\n", taos_errstr(archiver->taos));
      exit(1);
    }else{printf("successful!!!");}
    char* info = taos_get_server_info(archiver->taos);
    printf("server info: %s\n", info);
    info = taos_get_client_info(archiver->taos);
    printf("client info: %s\n", info);

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
    TAOS_RES* result;
    char str[256];
    while (true)
    {
        //epicsMutexMustLock(Archiver->ring_buffer->readLock);
        if(fifoRead(Archiver->ring_buffer, &data)!= FIFO_EMPTY)
        {    
            printf("-----------------------\n");
            printf("archive thread called\n");
            printf("pvname: %s\n",data.pvname);
            printf("new value is %s\n",val2str (data.data, data.type,0));
            printf("%s\n",dbr2str (data.data, data.type));
            printf("-----------------------\n");
        /*-------------这里增加写入TDengine的代码----------------
            //result = taos_query(Archiver->taos,str);
            //char* errstr = taos_errstr(result);
            //printf("query sql: %s \n query result: %s \n", str, errstr);
            //taos_free_result(result);

            要求：1. 数据库连接中断时可以自动重新连接
                 2. 把时间戳、警报状态也一起存储.(具体获得方法展开dbr2str里查看）
        //----------------------------------------------------*/
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
		10000, (EPICSTHREADFUNC*)archive_thread, (void *)Archiver)
		== (epicsThreadId) 0) {
		printf ("ArchiverTask spawn error\n");
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

fifo_error fifoInitial(FIFO * fifo,int fifo_size)
{
    
    printf("setup buff\n");
    //fifo->buff = (ARCHIVE_ELEMENT *) mallocMustSucceed(fifo_size, sizeof(ARCHIVE_ELEMENT));
    printf("setup buff\n");

    if (fifo->buff==NULL)
    {
        printf("calloc erroe!\n");
        return FIFO_ERROR;
    }
    
    fifo->read_position=0;
    fifo->write_position=0;
    fifo->max_size=fifo_size;
    fifo->fifoLock = epicsMutexMustCreate();
    //fifo->readLock = epicsMutexMustCreate();
  
    if (fifo->fifoLock==0)
    {
       printf("faild to create mutex!\n");
       return FIFO_ERROR;
    }
    return FIFO_OK;
}


fifo_error fifoWrite(FIFO *fifo, ARCHIVE_ELEMENT data)
{  
    epicsMutexMustLock(fifo->fifoLock);
    fifo_error rtn;
    int in=fifo->write_position;
    int out = fifo->read_position;
    fifo->buff[in]=data;

    printf("-----------------------\n");
    printf("fifo write called\n");
    printf("pvname: %s\n",fifo->buff->pvname);
    printf("input index: %d, outpit index: %d\n", in ,out);
    printf("new value is %s\n",val2str (fifo->buff[in].data, data.type,0));
    printf("-----------------------\n");

    if (in == fifo->max_size - 1 )
    {
        fifo->write_position = 0;
        rtn = FIFO_OK;
    }
    if (in < fifo->max_size - 1 )
    {
        fifo->write_position= in+1;
        rtn = FIFO_OK;
    }
    if (in > fifo->max_size - 1 )
    {
        fifo->write_position=in+1;
        rtn = FIFO_ERROR;
    }
    epicsMutexUnlock(fifo->fifoLock);
    return rtn;
}

fifo_error fifoRead(FIFO *fifo, ARCHIVE_ELEMENT *data )
{
    epicsMutexMustLock(fifo->fifoLock);
    int in = fifo->write_position;
    int out = fifo->read_position;
    fifo_error rtn;
    
    //printf("fifo write called\n");

    if (out == in)
    {
        rtn = FIFO_EMPTY;
    }
    else
    {
        memcpy(data,fifo->buff+out, sizeof(ARCHIVE_ELEMENT));

        if (out == fifo->max_size - 1)
        {
            fifo->read_position = 0;
            rtn = FIFO_OK;
        }
        if (out < fifo->max_size - 1)
        {
            fifo->read_position = out +1;
            rtn = FIFO_OK;
        }
         if (out > fifo->max_size - 1)
        {
            rtn = FIFO_ERROR;
        }
    printf("-----------------------\n");
    printf("fifo read called\n");
    printf("input index: %d, outpit index: %d\n", in ,out);
    printf("read value is %s\n",val2str (data->data, data->type,0));
    printf("-----------------------\n");
    }
    epicsMutexUnlock(fifo->fifoLock);
    return rtn;
}
