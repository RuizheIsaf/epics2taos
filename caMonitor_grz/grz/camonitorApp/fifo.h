#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#ifndef FIFO_H
#define FIFO_H
typedef struct info{
    char messge[512];  
    int data_len;
}info_t;

struct _node {
	void *data;
	struct _node * next;
};
typedef struct _node node;
  
struct _end_q {
	node * first;
	node * last;
};
typedef struct _end_q queue;

struct _stru {
    int number;
    int thread_no;
};
// create and return the queue
queue * queue_factory(void);

// destory the queue (free all the memory associate with the que even the data)
void queue_destroy(queue * que);


// enque the data into queue
// data is expected to a pointer to a heap allocated memory
int enque(queue * que, void * data);

// return the data from the que (FIFO)
// and free up all the internally allocated memory
// but the user have to free the returning data pointer
void * deque(queue * que);
int show_all_info(queue * que);
#endif
