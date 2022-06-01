/*caMonitor.c*/

/* This example accepts the name of a file containing a list of pvs to monitor.
 * It prints a message for all ca events: connection, access rights and monitor.
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cadef.h"
#include "dbDefs.h"
#include "epicsString.h"
#include "cantProceed.h"

#include <taos.h>//database
#include <time.h>//time
#include "fifo.h"//first in first out
#include <unistd.h>
//#include <iostream>
//#include <queue>

#define MAX_PV 1000
#define MAX_PV_NAME_LEN 40

typedef struct{
    char        value[20];
    chid        mychid;
    evid        myevid;
} MYNODE;

int show_all_info();





//global taos point by ruizhe
TAOS* taos;
TAOS_RES* result;
queue * q;


static void printChidInfo(chid chid, char *message)
{
    printf("\n%s\n",message);
    printf("pv: %s  type(%d) nelements(%ld) host(%s)",
        ca_name(chid),ca_field_type(chid),ca_element_count(chid),
        ca_host_name(chid));
    printf(" read(%d) write(%d) state(%d)\n",
        ca_read_access(chid),ca_write_access(chid),ca_state(chid));
}


static void printChidInfo_taos(chid chid, char *message)
{
    //get time start
    time_t rawtime;
    struct tm *info;
   	char time_cur[40];
 
   	time( &rawtime );
 	
   	info = localtime( &rawtime );
 
   	strftime(time_cur, 80, "%Y-%m-%d %H:%M:%S", info);
   	
   	//get time finish

    // char str1[100];
    // printf("\n%s\n",message);
    // sprintf(str1, "\'pv: %s  type(%d) nelements(%ld) host(%s) read(%d) write(%d) state(%d)\'",
    //     ca_name(chid),ca_field_type(chid),ca_element_count(chid),
    //     ca_host_name(chid), ca_read_access(chid),ca_write_access(chid),ca_state(chid));
    
    char str[256];
    //char str1[256];
    
    
    // result = taos_query(taos, "create database if not exists status;");
    // taos_free_result(result);
    // usleep(100000);
    // taos_select_db(taos, "status");

    // sprintf(str, "create table if not exists %s (ts TIMESTAMP, type INT, nelements BIGINT, host NCHAR(200), read INT, write INT, st INT);", ca_name(chid));
    // //printf("str_ct: %s \n ", str_ct);
    // sprintf(str1, "insert into status.%s values (\'%s\', %d, %ld, \'%s\', %d, %d, %d );",ca_name(chid), time_cur, ca_field_type(chid), ca_element_count(chid), ca_host_name(chid), ca_read_access(chid),ca_write_access(chid),ca_state(chid));
    // //printf("str_iv: %s \n ", str_iv);
    // strcat(str, str1);

    sprintf(str, "insert into status.%s using status.pv_st tags(0) values (\'%s\', %d, %ld, \'%s\', %d, %d, %d );",ca_name(chid), time_cur, ca_field_type(chid), ca_element_count(chid), ca_host_name(chid), ca_read_access(chid),ca_write_access(chid),ca_state(chid));

    printf("str: %s \n ", str);
    result = taos_query(taos, str);
    char* errstr = taos_errstr(result);
    printf("query sql: %s \n query result: %s \n", str, errstr);
    taos_free_result(result);    
}

static void exceptionCallback(struct exception_handler_args args)
{
    chid        chid = args.chid;
    long        stat = args.stat; /* Channel access status code*/
    const char  *channel;
    static char *noname = "unknown";

    channel = (chid ? ca_name(chid) : noname);


    if(chid) printChidInfo(chid,"exceptionCallback");
    printf("exceptionCallback stat %s channel %s\n",
        ca_message(stat),channel);
}

static void connectionCallback(struct connection_handler_args args)
{
    chid        chid = args.chid;

    printChidInfo_taos(chid,"connectionCallback");

    
    
    /*
       how to send error info to 192.168.20.6(zhongkong ip)?by  ruizhe
    */
}


static void accessRightsCallback(struct access_rights_handler_args args)
{
    chid        chid = args.chid;

    printChidInfo(chid,"accessRightsCallback");
}



static void eventCallback(struct event_handler_args eha)
{

    //get local time by  ruizhe
    time_t rawtime;
   	struct tm *info;
   	char time_cur[40];
 
   	time( &rawtime );
 	
   	info = localtime( &rawtime );
 
   	strftime(time_cur, 80, "%Y-%m-%d %H:%M:%S", info);
   	
   	//get time finish
   	
    chid        chid = eha.chid;

    if(eha.status!=ECA_NORMAL) {
        printChidInfo(chid,"eventCallback");
    } else {
        char    *pdata = (char *)eha.dbr; 
        printf("Event Callback: %s = %s\n",ca_name(eha.chid),pdata);
        
        
         	
   	    //insert sql by  ruizhe
        char str[128];
        // char str1[128];

        // sprintf(str, "create table if not exists %s (ts TIMESTAMP, val INT); \n",ca_name(eha.chid));
        // //printf("\n %s", str_ct);
        // sprintf(str1, "insert into pvtest.%s values (\'%s\', %s); \n",ca_name(eha.chid), time_cur, pdata);
        // // //printf("\n %s", str_iv);
        // strcat(str, str1);
        sprintf(str, "insert into pvs.%s using pvs.pv_val tags(0) values (\'%s\', %s); \n",ca_name(eha.chid), time_cur, pdata);

        printf("str: %s \n ", str);

        info_t* message = (info_t*)malloc(sizeof(info_t));
        sprintf(message->messge,  str);//send string to message 
        enque(q, (void *)message);
        
        
        printf("print all info: \n");
        show_all_info(q);
        
        //int data_cur = (int)eha.dbr;
        //printf("data_cur = %d", data_cur);
    }
}



//thread read function by ruizhe
void* thread_read(int* number) 
{ 
while(1){
    void * data;
    while((data = deque(q)) != NULL) {
        char * string = ((info_t *)data)->messge;
//        printf("DeQued : %s, @%p\n", string, data);
        
        
        printf("now is in thread: %d \n", *number);

        // result = taos_query(taos, "create database if not exists pvtest;");
        // taos_free_result(result);
        // usleep(100000);
        // taos_select_db(taos, "pvtest");

        printf("\n %s \n", string);
        result = taos_query(taos,string);
        char* errstr = taos_errstr(result);
        printf("query sql: %s \n query result: %s \n", string, errstr);
        taos_free_result(result);
          
        free(data);
    }
}
return NULL;
}



int main(int argc,char **argv)
{

    q = queue_factory();
    if (q != NULL){
		printf("yes i can!!!");
	}
    char *filename;
    int         npv = 0;
    MYNODE      *pmynode[MAX_PV];
    char        *pname[MAX_PV];
    int i;
    char        tempStr[MAX_PV_NAME_LEN];
    char        *pstr;
    FILE        *fp;


  //taos connect begin   by  ruizhe
    const char* host = "127.0.0.1";
    const char* user = "root";
    const char* passwd = "taosdata";

    taos_options(TSDB_OPTION_TIMEZONE, "GMT-8");
    taos = taos_connect(host, user, passwd, "", 0);
    if (taos == NULL) {
      printf("\033[31mfailed to connect to db, reason:%s\033[0m\n", taos_errstr(taos));
      exit(1);
    }else{printf("successful!!!");}

    char* info = taos_get_server_info(taos);
    printf("server info: %s\n", info);
    info = taos_get_client_info(taos);
    printf("client info: %s\n", info);
  
    //taos connect end
  
    //??
	/*  change priority
    pthread_attr_t attr1;
    pthread_attr_init(&attr1);
    pthread_attr_setinheritsched(&attr1, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr1, SCHED_OTHER);
    struct sched_param param1;
    param1.sched_priority = 0;
    pthread_attr_setschedparam(&attr1, &param1);
    */
    int arg[5];
    pthread_t threads[5];
    for (i=0;i<5;i++){
        arg[i] = i;
        //pthread_t tid; 
        pthread_create(threads+i, NULL, thread_read, &arg[i]); 
    }
	
    if (argc != 2) {
        fprintf(stderr,"usage: caMonitor filename\n");
        exit(1);
    }
    filename = argv[1];
    fp = fopen(filename,"r");
    if (!fp) {
        perror("fopen failed");
        return(1);
    }
    while (npv < MAX_PV) {
        size_t len;

        pstr = fgets(tempStr, MAX_PV_NAME_LEN, fp);
        if (!pstr) break;

        len = strlen(pstr);
        if (len <= 1) continue;

        pstr[len - 1] = '\0'; /* Strip newline */
        pname[npv] = epicsStrDup(pstr);
        pmynode[npv] = callocMustSucceed(1, sizeof(MYNODE), "caMonitor");
        npv++;
    }
    fclose(fp);

    SEVCHK(ca_context_create(ca_disable_preemptive_callback),"ca_context_create");
    SEVCHK(ca_add_exception_event(exceptionCallback,NULL),
        "ca_add_exception_event");
    for (i=0; i<npv; i++) {
        SEVCHK(ca_create_channel(pname[i],connectionCallback,
                pmynode[i],20,&pmynode[i]->mychid),
                "ca_create_channel");
        SEVCHK(ca_replace_access_rights_event(pmynode[i]->mychid,
                accessRightsCallback),
                "ca_replace_access_rights_event");
        SEVCHK(ca_create_subscription(DBR_STRING,1,pmynode[i]->mychid,
                DBE_VALUE,eventCallback,pmynode[i],&pmynode[i]->myevid),
                "ca_create_subscription");
    }
    /*Should never return from following call*/
    SEVCHK(ca_pend_event(0.0),"ca_pend_event");
    return 0;
}
