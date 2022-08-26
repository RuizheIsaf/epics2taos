#include "taos_interface.h"
#include "archiver.h"


TAOS* TaosConnect()
{
    char  **fileData = NULL;
    int lines = 0;
    struct ConfigInfo* info = NULL;
    loadFile_configFile("./config.ini", &fileData, &lines);
    parseFile_configFile(fileData, lines, &info);
    char *host = getInfo_configFile("host", info, lines);
    char *user = getInfo_configFile("user", info, lines);
    char *passwd = getInfo_configFile("passwd", info, lines);

    TAOS *taos;
    taos_options(TSDB_OPTION_TIMEZONE, "GMT-8");
    taos = taos_connect(host, user, passwd, "", 0);
    if (taos == NULL) {
      printf("\033[31mfailed to connect to db, reason: %s\033[0m\n", taos_errstr(taos));
      

      //******************
      //此处加一个系统记录。
      //******************
      syslog(LOG_USER|LOG_INFO,"TDengine connection error\n"); 

      exit(1);
    }else{printf("successful!!!");}
    char* taosinfo = taos_get_server_info(taos);
    printf("server info: %s\n", taosinfo);
    taosinfo = taos_get_client_info(taos);
    printf("client info: %s\n", taosinfo);

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
    TAOS_RES* result;

    result = taos_query(Archiver->taos, "create database if not exists status;");
    taos_free_result(result);
    result = taos_query(Archiver->taos, "use status;");
    taos_free_result(result);
    result = taos_query(Archiver->taos, "create stable if not exists st(ts TIMESTAMP, val INT) tags(groupId INT);");
    taos_free_result(result);
    sprintf(sql, "insert into status.`%s` using status.st tags(0) values (\'%s\', %d) \n" , ppv->name, timeText, status);
    result = taos_query(Archiver->taos, sql);
    int errno = taos_errno(result);
    if (result == NULL || errno != 0) {//如果taos_errno返回0说明执行成功
        printf("failed to insert row: %s, reason: %s\n", sql, taos_errstr(result));
        syslog(LOG_USER|LOG_INFO,"TDengine insert error\n");
        taos_free_result(result);
        //exit(1);

        //通过返回的errono判断是否断线，如果断线则重新连接
        //错误代码参照：https://www.bookstack.cn/read/TDengin-2.0-zh/9436ce1aea0b27a2.md
        //“Unable to establish connection”：-2147483637
        //“Disconnected from service”：-2147483117
        if(errno == -2147483637 || errno == -2147483117) {
        
	        syslog(LOG_USER|LOG_INFO,"TDengine disconnected error\n");//将错误写入日志
            while(Archiver->taos == NULL) {
			    
			    Archiver->taos = TaosConnect();//如果连接中断，重新连接
                sleep(5);
		    }     
        }


    } else {
        #ifdef DEBUG
        printf("insert row: %s result: success\n", sql);
        #endif
    }

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
    char sql[256];
    //char sql2[256];
    char* errstr;

    result = taos_query(Archiver->taos, "create database if not exists pvs;");
    taos_free_result(result);
    result = taos_query(Archiver->taos, "use pvs;");
    taos_free_result(result);
    result = taos_query(Archiver->taos, "create stable if not exists pv(ts TIMESTAMP, val FLOAT, status NCHAR(20), severity NCHAR(20)) tags(groupId INT);");
    taos_free_result(result);
    sprintf(sql, "insert into pvs.`%s` using pvs.pv tags(0) values (\'%s\', \'%s\', \'%s\', \'%s\'); \n ", data.pvname, ts, value, status, severity);
    
    result = taos_query(Archiver->taos, sql);
    int errno = taos_errno(result);
    if (result == NULL || errno != 0) {//如果taos_errno返回0说明执行成功
        printf("failed to insert row: %s, reason: %s\n", sql, taos_errstr(result));
        syslog(LOG_USER|LOG_INFO,"TDengine insert error\n");
        taos_free_result(result);
        //exit(1);
        //通过返回的errono判断是否断线，如果断线则重新连接
        //错误代码参照：https://www.bookstack.cn/read/TDengin-2.0-zh/9436ce1aea0b27a2.md
        //“Unable to establish connection”：-2147483637
        //“Disconnected from service”：-2147483117
        if(errno == -2147483637 || errno == -2147483117) {
        
	        syslog(LOG_USER|LOG_INFO,"TDengine disconnected error\n");//将错误写入日志
            while(Archiver->taos == NULL) {
			    
			    Archiver->taos = TaosConnect();//如果连接中断，重新连接
                sleep(5);
		    }     
        }


    } else {
        #ifdef DEBUG
        printf("insert row: %s result: success\n", sql);
        #endif
    }
    // result = taos_query(Archiver->taos, sql);
    // errstr = taos_errstr(result);
    // printf("query sql: %s \n query result: %s \n", sql, errstr);
    taos_free_result(result);
    // sprintf(sql2, "insert into status.`%s` using status.st tags(0) values (\'%s\', 1) \n" , data.pvname, ts);
    // result = taos_query(taos, sql2);
    // errstr = taos_errstr(result);
    // printf("query sql: %s \n query result: %s \n", sql2, errstr);
    // taos_free_result(result);
    //printf("-----------------------\n");
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
    result = taos_query(Archiver->taos, "create database if not exists monitor;");
    taos_free_result(result);
    result = taos_query(Archiver->taos, "use monitor;");
    taos_free_result(result);
    result = taos_query(Archiver->taos, "CREATE TABLE IF NOT EXISTS monitor_pv(ts TIMESTAMP, callbackcounts BIGINT, npvon  INT, npvoff INT);");
    taos_free_result(result);
    sprintf(sql, "insert into monitor.monitor_pv values (\'%s\', %d, %d, %d) \n" , timeText, callBackCounts, nPvOn, nPvOff);
    
    result = taos_query(Archiver->taos, sql);
    int errno = taos_errno(result);
    if (result == NULL || errno != 0) {//如果taos_errno返回0说明执行成功
        printf("failed to insert row: %s, reason: %s\n", sql, taos_errstr(result));
        syslog(LOG_USER|LOG_INFO,"TDengine insert error\n");
        taos_free_result(result);
        //exit(1);
        //通过返回的errono判断是否断线，如果断线则重新连接
        //错误代码参照：https://www.bookstack.cn/read/TDengin-2.0-zh/9436ce1aea0b27a2.md
        //“Unable to establish connection”：-2147483637
        //“Disconnected from service”：-2147483117
        if(errno == -2147483637 || errno == -2147483117) {
        
	        syslog(LOG_USER|LOG_INFO,"TDengine disconnected error\n");//将错误写入日志
            while(Archiver->taos == NULL) {
			    
			    Archiver->taos = TaosConnect();//如果连接中断，重新连接
                sleep(5);
		    }     
        }



    } else {
        #ifdef DEBUG
        printf("insert row: %s result: success\n", sql);
        #endif
    }
    // result = taos_query(Archiver->taos, sql);
    // errstr = taos_errstr(result);
    // printf("query sql: %s \n query result: %s \n", sql, errstr);
    taos_free_result(result);
}