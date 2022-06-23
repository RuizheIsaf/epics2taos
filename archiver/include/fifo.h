#include "headers.h"

fifo_error fifoInitial(FIFO * fifo,int fifo_size);
fifo_error fifoWrite(FIFO *fifo, ARCHIVE_ELEMENT data);
fifo_error fifoRead(FIFO *fifo,ARCHIVE_ELEMENT *data);
//fifo_error fifoReadBulk(FIFO *fifo, int read_size, ARCHIVE_ELEMENT *readbuff);



