#include "taos_interface.h"
#include "archiver.h"


TAOS* TaosConnect(char* host, char* user,char* passwd)
{
    TAOS *taos;
    taos_options(TSDB_OPTION_TIMEZONE, "GMT-8");
    taos = taos_connect(host, user, passwd, "", 0);
    if (taos == NULL) {
      printf("\033[31mfailed to connect to db, reason:%s\033[0m\n", taos_errstr(taos));
      
      //******************
      //此处加一个系统记录。
      //******************
      syslog(LOG_USER|LOG_INFO,"TDengine connection error\n"); 

      exit(1);
    }else{printf("successful!!!");}
    char* info = taos_get_server_info(taos);
    printf("server info: %s\n", info);
    info = taos_get_client_info(taos);
    printf("client info: %s\n", info);

    return taos;

}

int PVStatus2TD(TAOS * taos, pv * ppv, int status)
{
    //获取时间戳
    char timeText[58];
    char timeFormatStr[30] = "%Y-%m-%d %H:%M:%S.%06f";
    epicsTimeStamp tsNow;
    epicsTimeGetCurrent(&tsNow);
    epicsTimeToStrftime(timeText, 28, timeFormatStr, &tsNow);

    //准备插入数据的sql
    char sql[256];
    char* errstr;
    sprintf(sql, "insert into status.`%s` using status.st tags(0) values (\'%s\', %d) \n" , ppv->name, timeText, status);
    //printf("query sql: %s ", sql);
    TAOS_RES* result;
    result = taos_query(Archiver->taos, sql);
    errstr = taos_errstr(result);
    printf("query sql: %s \n query result: %s \n", sql, errstr);
    taos_free_result(result);
}

int Pv2TD(TAOS * taos, ARCHIVE_ELEMENT data)
{
    TAOS_RES* result;
    char str[256];
    char* dbrstr = dbr2str (data.data, data.type);
    char* buf[3], *p;
    int i = 0;
    p = NULL;
    p = strtok(dbrstr, ",");
    while(p) {
        buf[i] = p;
        i++;
        p = strtok(NULL, ",");
    }
    //将分割好的字符串分别赋值给ts, status, severity
    char* ts = buf[0];
    char* status = buf[1];
    char* severity = buf[2];
    char* value = val2str (data.data, data.type,0);

    //printf(dbr2str (data.data, data.type));
    //准备sql语句并往tdengine里写入数据，sql1插入数值变化，sql2插入状态变化
    char sql1[256];
    //char sql2[256];
    char* errstr;
    sprintf(sql1, "insert into pvs.`%s` using pvs.pv tags(0) values (\'%s\', \'%s\', \'%s\', \'%s\'); \n ", data.pvname, ts, value, status, severity);
    result = taos_query(Archiver->taos, sql1);
    errstr = taos_errstr(result);
    printf("query sql: %s \n query result: %s \n", sql1, errstr);
    taos_free_result(result);
    // sprintf(sql2, "insert into status.`%s` using status.st tags(0) values (\'%s\', 1) \n" , data.pvname, ts);
    // result = taos_query(taos, sql2);
    // errstr = taos_errstr(result);
    // printf("query sql: %s \n query result: %s \n", sql2, errstr);
    // taos_free_result(result);
    printf("-----------------------\n");
}

int HB2TD(TAOS * taos, int callBackCounts, int nPvOn, int nPvOff)
{
    TAOS_RES* result;
    char timeText[58];
    char timeFormatStr[30] = "%Y-%m-%d %H:%M:%S.%06f";
    epicsTimeStamp tsNow;
    epicsTimeGetCurrent(&tsNow);
    epicsTimeToStrftime(timeText, 28, timeFormatStr, &tsNow);
    char sql[256];
    char* errstr;
    sprintf(sql, "insert into monitor.monitor_pv values (\'%s\', %d, %d, %d) \n" , timeText, callBackCounts, nPvOn, nPvOff);
    result = taos_query(Archiver->taos, sql);
    errstr = taos_errstr(result);
    printf("query sql: %s \n query result: %s \n", sql, errstr);
    taos_free_result(result);
}