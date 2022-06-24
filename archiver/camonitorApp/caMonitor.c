/*caMonitor.c*/

/* This example accepts the name of a file containing a list of pvs to monitor.
 * It prints a message for all ca events: connection, access rights and monitor.
 */
#include <stdio.h>
#include <epicsStdlib.h>
#include <string.h>
#include "epicsVersion.h"

#include <cadef.h>
#include <epicsGetopt.h>
#include "archiver.h"


#define VALID_DOUBLE_DIGITS 18  /* Max usable precision for a double */
static unsigned long reqElems = 0;
static unsigned long eventMask = DBE_VALUE | DBE_ALARM;   /* Event mask used */
static int floatAsString = 0;                             /* Flag: fetch floats as string */
static int nConn = 0;                                     /* Number of connected PVs */

ARCHIVER* Archiver; 

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
    //TAOS_RES* result;
   	time( &rawtime );
 	
   	info = localtime( &rawtime );
 
   	strftime(time_cur, 80, "%Y-%m-%d %H:%M:%S", info);
   	
    char str[256];
    sprintf(str, "insert into status.`%s` using status.pv_st tags(0) values (\'%s\', %d, %ld, \'%s\', %d, %d, %d );",ca_name(chid), time_cur, ca_field_type(chid), ca_element_count(chid), ca_host_name(chid), ca_read_access(chid),ca_write_access(chid),ca_state(chid));
    printf("str: %s \n ", str);
    /*-----------------------
        将连接状态变化写入TDengine
    //result = taos_query(taos, str);
    //char* errstr = taos_errstr(result);
   // printf("query sql: %s \n query result: %s \n", str, errstr);
    //taos_free_result(result);    
        数据库连接中断时自动重新连接（在独立的函数中实现。）
    ------------------------------*/
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



static void accessRightsCallback(struct access_rights_handler_args args)
{
    chid        chid = args.chid;

    printChidInfo(chid,"accessRightsCallback");
}



static void eventCallback(struct event_handler_args eha)
{
    pv* pv = eha.usr;

    pv->status = eha.status;
    if (eha.status == ECA_NORMAL)
    {
        //pv->dbrType = eha.type;
        //pv->nElems = eha.count;
        //pv->value = (void *) eha.dbr;    /* casting away const */
        //print_time_val_sts(pv, reqElems);
        //printf("display in camonitor: %s\n",  val2str((void*)eha.dbr, eha.type,0));
        //fflush(stdout);
        //pv->value = NULL;
        archive_pv(eha);   
    }
}

/*-----------------------
     caMonitor模版的例子这里也是错的。
     PV上线时才能得到正确的类型信息，把订阅功能放在connectionCallback里可以连接启动时不在线的PV。
     当PV的类型改变后，比如认为修改了IOC后重启。。程序能以正确的类型进行订阅。
------------------------*/

static void connectionCallback(struct connection_handler_args args)
{
    pv *ppv = ( pv * ) ca_puser ( args.chid );
    if (args.op == CA_OP_CONN_UP ) {
        nConn++;

        if (ppv->onceConnected && ppv->dbfType != ca_field_type(ppv->chid)) {
            /* Data type has changed. Rebuild connection with new type. */
            ca_clear_subscription(ppv->evid);
            ppv->evid = NULL;
        }

        if (!ppv->evid) {
            ppv->onceConnected = 1;
                                /* Set up pv structure */
                                /* Get natural type and array count */
            ppv->dbfType = ca_field_type(ppv->chid);
            ppv->dbrType = dbf_type_to_DBR_TIME(ppv->dbfType); /* Use native type */
            if (dbr_type_is_ENUM(ppv->dbrType))                /* Enums honour -n option */
            {
                if (enumAsNr) ppv->dbrType = DBR_TIME_INT;
                else          ppv->dbrType = DBR_TIME_STRING;
            }
            else if (floatAsString &&
                     (dbr_type_is_FLOAT(ppv->dbrType) || dbr_type_is_DOUBLE(ppv->dbrType)))
            {
                ppv->dbrType = DBR_TIME_STRING;
            }
                                /* Set request count */
            ppv->nElems   = ca_element_count(ppv->chid);
            ppv->reqElems = reqElems > ppv->nElems ? ppv->nElems : reqElems;

                                /* Issue CA request */
            /* install monitor once with first connect */
            ppv->status = ca_create_subscription(ppv->dbrType,
                                                ppv->reqElems,
                                                ppv->chid,
                                                eventMask,
                                                eventCallback,
                                                (void*)ppv,
                                                &ppv->evid);
        }
    }
    else if ( args.op == CA_OP_CONN_DOWN ) {
        nConn--;
        ppv->status = ECA_DISCONN;
        print_time_val_sts(ppv, reqElems);
    }

}



int main(int argc,char **argv)
{
   
    char *filename;
    int         npv = 0;
    pv          *pmynode[MAX_PV];
    char        *pname[MAX_PV];
    int i;
    char        tempStr[MAX_PV_NAME_LEN];
    char        *pstr;
    FILE        *fp;
    printf("Start!\n");

    Archiver = archive_initial();
    printf("archiver initilized!\n");
    
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
        pmynode[npv] = (pv*)callocMustSucceed(1, sizeof(pv), "caMonitor");
        pmynode[npv]->name=epicsStrDup(pstr);
        npv++;
    }
    fclose(fp);
    Archiver->nodelist = pmynode;
    printf("Setup monitor!\n");
    start_archive_thread(Archiver);          //启动读取线程，将fifo中的数据读出来写入TDengine

    printf("archiver thread started!\n");
    SEVCHK(ca_context_create(ca_disable_preemptive_callback),"ca_context_create");
    SEVCHK(ca_add_exception_event(exceptionCallback,NULL),
        "ca_add_exception_event");
    for (i=0; i<npv; i++) {
        SEVCHK(ca_create_channel(pmynode[i]->name,connectionCallback,
                (void*)pmynode[i],20,&pmynode[i]->chid),
                "ca_create_channel");
        SEVCHK(ca_replace_access_rights_event(pmynode[i]->chid,
                accessRightsCallback),
                "ca_replace_access_rights_event");
        
    }
    /*Should never return from following call*/
    SEVCHK(ca_pend_event(0.0),"ca_pend_event");
    return 0;
}
