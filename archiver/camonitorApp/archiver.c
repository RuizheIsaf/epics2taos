
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <alarm.h>
#include <epicsString.h>

#include "archiver.h"


ARCHIVE_ERROR archive_initial(ARCHIVER *archiver)
{
    fifoInitial(archiver->ring_buffer,BUFF_LENGTH);    //初始化缓存

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
}

ARCHIVE_ERROR archive_pv(evargs eha)
{
    ARCHIVE_ELEMENT newdata;
    ARCHIVER *archiver=(ARCHIVER *)ca_puser(eha.chid);
    strcpy(&newdata.pvname,ca_name(eha.chid));
    ///newdata.pvname = ca_name(eha.chid);
    newdata.type = eha.type;
    newdata.count = eha.count;

    switch (eha.type) {
        
        case DBR_TIME_STRING:

            memcpy(&newdata.value, &((struct dbr_time_string *)eha.dbr)->value, sizeof(dbr_string_t));
            newdata.status = ((struct dbr_time_string *)eha.dbr)->status;
            newdata.severity = ((struct dbr_time_string *)eha.dbr)->severity;
            newdata.stamp=((struct dbr_time_string *)eha.dbr)->stamp;
            break;
        case DBR_TIME_SHORT:
            //PRN_TIME_VAL_STS(dbr_time_short, DBR_TIME_SHORT);
            memcpy(&newdata.value,&((struct dbr_time_short*)eha.dbr)->value,sizeof(dbr_short_t));
            newdata.status = ((struct dbr_time_short*)eha.dbr)->status;
            newdata.severity =((struct dbr_time_short*)eha.dbr)->severity;
            newdata.stamp=((struct dbr_time_short*)eha.dbr)->stamp;
            break;
        case DBR_TIME_FLOAT:
            //PRN_TIME_VAL_STS(struct dbr_time_float, DBR_TIME_FLOAT);
            memcpy(&newdata.value, &((struct dbr_time_float*)eha.dbr)->value,sizeof(dbr_float_t));
            newdata.status = ((struct dbr_time_float*)eha.dbr)->status;
            newdata.severity = ((struct dbr_time_float*)eha.dbr)->severity;
            newdata.stamp=((struct dbr_time_float*)eha.dbr)->stamp;
            break;
        case DBR_TIME_ENUM:
            //PRN_TIME_VAL_STS(dbr_time_enum, DBR_TIME_ENUM);
            memcpy(&newdata.value,&((struct dbr_time_enum*)eha.dbr)->value,sizeof(dbr_enum_t));
            newdata.status = ((struct dbr_time_enum*)eha.dbr)->status;
            newdata.severity = ((struct dbr_time_enum*)eha.dbr)->severity;
            newdata.stamp=((struct dbr_time_enum*)eha.dbr)->stamp;
            break;
        case DBR_TIME_CHAR:
            //PRN_TIME_VAL_STS(dbr_time_char, DBR_TIME_CHAR);
            memcpy(&newdata.value,&((struct dbr_time_char*)eha.dbr)->value,sizeof(dbr_char_t));
            newdata.status = ((struct dbr_time_char*)eha.dbr)->status;
            newdata.severity = ((struct dbr_time_char*)eha.dbr)->severity;
            newdata.stamp=((struct dbr_time_char*)eha.dbr)->stamp;
            break;
        case DBR_TIME_LONG:
           // PRN_TIME_VAL_STS(struct dbr_time_long, DBR_TIME_LONG);
            memcpy(&newdata.value,&((struct dbr_time_long*)eha.dbr)->value,sizeof(dbr_long_t));
            newdata.status = ((struct dbr_time_long*)eha.dbr)->status;
            newdata.severity =((struct dbr_time_long*)eha.dbr)->severity;
            newdata.stamp=((struct dbr_time_long*)eha.dbr)->stamp;
            break;
        case DBR_TIME_DOUBLE:
            //PRN_TIME_VAL_STS(dbr_time_double, DBR_TIME_DOUBLE);
            memcpy(&newdata.value,&((struct dbr_time_double*)eha.dbr)->value,sizeof(dbr_double_t));
            newdata.status = ((struct dbr_time_double*)eha.dbr)->status;
            newdata.severity = ((struct dbr_time_double*)eha.dbr)->severity;
            newdata.stamp=((struct dbr_time_double*)eha.dbr)->stamp;
            break;
        default: printf("can't print data type\n");
        }
        
        if(fifoWrite(archiver->ring_buffer, newdata)!=FIFO_OK)
        {
            printf("fifo write error!\n");
    }   
}


void archive_thread(ARCHIVER *parchiver)
{ 
    ARCHIVER *archiver = (ARCHIVER *)parchiver;
    ARCHIVE_ELEMENT data;
    TAOS_RES* result;
    char str[256];
    while (true)
    {
        while ( fifoRead(archiver->ring_buffer, &data)!= FIFO_EMPTY)
        {    
            switch (data.type) {
                case DBR_TIME_STRING:
                   //sprintf(str, "insert into pvs.`%s` using pvs.pv_val tags(0) values (\'%s\', %s); \n",data.pvname, data.stamp, (char *)data.value);
                   // printf("str: %s \n ", str);
                    break;
                case DBR_TIME_SHORT:
                    //sprintf(str, "insert into pvs.`%s` using pvs.pv_val tags(0) values (\'%s\', %s); \n",data.pvname,data.stamp, (char *)data.value);
                    //printf("str: %s \n ", str);
                    break;
                case DBR_TIME_FLOAT:
                    //msprintf(str, "insert into pvs.`%s` using pvs.pv_val tags(0) values (\'%s\', %s); \n",data.pvname,data.stamp,(char *)data.value);
                    //printf("str: %s \n ", str);
                    break;
                case DBR_TIME_ENUM:
                    //sprintf(str, "insert into pvs.`%s` using pvs.pv_val tags(0) values (\'%s\', %s); \n",data.pvname,data.stamp, (char *)data.value);
                    //printf("str: %s \n ", str);
                    break;
                case DBR_TIME_CHAR:
                    //sprintf(str, "insert into pvs.`%s` using pvs.pv_val tags(0) values (\'%s\', %s); \n",data.pvname,data.stamp,(char *)data.value);
                    //printf("str: %s \n ", str);
                    break;
                case DBR_TIME_LONG:
                    //sprintf(str, "insert into pvs.`%s` using pvs.pv_val tags(0) values (\'%s\', %s); \n",data.pvname,data.stamp, (char *)data.value);
                    //printf("str: %s \n ", str);
                    break;
                case DBR_TIME_DOUBLE:
                    //sprintf(str, "insert into pvs.`%s` using pvs.pv_val tags(0) values (\'%s\', %s); \n",data.pvname,data.stamp, (char *)data.value);
                    //printf("str: %s \n ", str);
                    break;
                default: printf("can't print data type\n");
            }   
            result = taos_query(archiver->taos,str);
            char* errstr = taos_errstr(result);
            printf("query sql: %s \n query result: %s \n", str, errstr);
            taos_free_result(result);
        }
    }   
}

ARCHIVE_ERROR start_archive_thread(ARCHIVER *archiver)
{
    if (epicsThreadCreate("ArchiverTask", epicsThreadPriorityHigh,
		10000, (EPICSTHREADFUNC*)archive_thread, (void *)archiver)
		== (epicsThreadId) 0) {
		printf ("ArchiverTask spawn error\n");
		return -1;
	}
}


fifo_error fifoInitial(FIFO * fifo,int fifo_size)
{
    fifo->buff=(ARCHIVE_ELEMENT*)callocMustSucceed(fifo_size, sizeof(ARCHIVE_ELEMENT), "fifo");
    if (fifo->buff==NULL)
    {
        printf("calloc erroe!\n");
        return FIFO_ERROR;
    }
    
    fifo->read_position=0;
    fifo->write_position=0;
    fifo->fifoLock = epicsMutexCreate();
    if (fifo->fifoLock==0)
    {
       printf("faild to create mutex!\n");
       return FIFO_ERROR;
    }
    
    return FIFO_OK;
}


fifo_error fifoWrite(FIFO *fifo, ARCHIVE_ELEMENT data)
{
    fifo_error rtn;
    int in=fifo->write_position;
    int out = fifo->read_position;
    epicsMutexLock(fifo->fifoLock);
    fifo->buff[in]=data;
    
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
    int in=fifo->write_position;
    int out = fifo->read_position;
    fifo_error rtn;

    epicsMutexLock(fifo->fifoLock);

    if (out == in)
    {
        rtn = FIFO_EMPTY;
    }
    else
    {
        memcpy(data,fifo->buff+out, sizeof(ARCHIVE_ELEMENT));

        if (out = fifo->max_size - 1)
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
    }
     epicsMutexUnlock(fifo->fifoLock);
    return rtn;
}