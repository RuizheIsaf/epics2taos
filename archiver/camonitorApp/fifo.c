#include "archiver.h"


fifo_error fifoInitial(FIFO * fifo,int fifo_size)
{
    
    printf("setup buff\n");
    //fifo->buff = (ARCHIVE_ELEMENT *) mallocMustSucceed(fifo_size, sizeof(ARCHIVE_ELEMENT));
    printf("setup buff\n");

    if (fifo->buff==NULL)
    {
        printf("calloc error!\n");
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

    #ifdef DEBUG
    printf("-----------------------\n");
    printf("fifo write called\n");
    printf("pvname: %s\n",fifo->buff->pvname);
    printf("input index: %d, output index: %d\n", in ,out);
    printf("new value is %s\n",val2str (fifo->buff[in].data, data.type,0));
    printf("-----------------------\n");
    #endif

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
    #ifdef DEBUG
    printf("-----------------------\n");
    printf("fifo read called\n");
    printf("input index: %d, output index: %d\n", in ,out);
    printf("read value is %s\n",val2str (data->data, data->type,0));
    printf("-----------------------\n");
    #endif
    }
    epicsMutexUnlock(fifo->fifoLock);
    return rtn;
}