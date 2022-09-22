#include "taos_interface.h"
#include "archiver.h"
#include "math.h"


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

    result = taos_query(Archiver->taos, sql);
    int errno = taos_errno(result);
    if (result == NULL || errno != 0) {//如果taos_errno返回0说明执行成功
        printf("failed to insert row: %s, reason: %s\n", sql, taos_errstr(result));
        syslog(LOG_USER|LOG_INFO,"TDengine insert error\n");
        taos_free_result(result);
        //exit(1);

        if(errno == -2147482752) {//"Database not specified or available"，建库并且建超级表，之后再执行一遍插入
            printf("Database not specified or available\n");
            result = taos_query(Archiver->taos, "create database if not exists status;");
            taos_free_result(result);
            printf("Database status created!\n");
            result = taos_query(Archiver->taos, "use status;");
            taos_free_result(result);
            printf("Using database status...\n");
            result = taos_query(Archiver->taos, "create stable if not exists st(ts TIMESTAMP, val INT) tags(groupId INT);");
            taos_free_result(result);
            printf("Stable st created!\n");
            sprintf(sql, "insert into status.`%s` using status.st tags(0) values (\'%s\', %d) \n" , ppv->name, timeText, status);
            result = taos_query(Archiver->taos, sql);
            errno = taos_errno(result);
        } 
        if(errno == -2147482782) {//"Table does not exist"，有库没表，建超级表，之后再执行一次插入
            printf("Table does not exist\n");
            result = taos_query(Archiver->taos, "use status;");
            taos_free_result(result);
            printf("Using database status...\n");
            result = taos_query(Archiver->taos, "create stable if not exists st(ts TIMESTAMP, val INT) tags(groupId INT);");
            taos_free_result(result);
            printf("Stable st created!\n");
            sprintf(sql, "insert into status.`%s` using status.st tags(0) values (\'%s\', %d) \n" , ppv->name, timeText, status);
            result = taos_query(Archiver->taos, sql);
            errno = taos_errno(result);
        }

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
    /*
    TAOS_RES* result;
    epicsTimeStamp tsNow;
    epicsTimeGetCurrent(&tsNow);

    TAOS_STMT *stmt = taos_stmt_init(taos);
    const char *sql = "insert into ? using st tags(0) values (?, ?);";
    //const char *sql = "insert into ? using pv tags(0) values (?, ?, ?, ?);";
    int code  = taos_stmt_prepare(stmt,sql, 0);//0表示会自动判断sql语句的长度
    checkErrorCode(stmt, code, "failed to excute taos_stmt_prepare\n");

    unsigned long ts1 = tsNow.secPastEpoch;//uint类型 * 1000会溢出，先转为ulong型
    unsigned long ts2 = tsNow.nsec;
    //secPastEpoch时间跟unix时间差了1970到1990的这2年，即7305 * 24 * 60 * 60 s 
    //tdengine需要的时间戳以毫秒为单位的时间戳
    ts1 = (ts1  + 631152000) * 1000 + ts2 / 1000000;

    char tbname[64];
    sprintf(tbname, "`%s`", ppv->name);
    code = taos_select_db(taos, "status");
    //printf("status_error_code: %d", code);
    if (code != 0) {
        //database not exist
        printf("Database not specified or available\n");
        result = taos_query(Archiver->taos, "create database if not exists status;");
        taos_free_result(result);
        printf("Database pvs created!\n");
        result = taos_query(Archiver->taos, "use status;");
        taos_free_result(result);
        printf("Using database status...\n");
        result = taos_query(Archiver->taos, "create stable if not exists st(ts TIMESTAMP, val INT) tags(groupId INT);");
        taos_free_result(result);
        printf("Stable st created!\n");
    }
    //printf("tbname: %s\n", tbname);
    code = taos_stmt_set_tbname(stmt, tbname);
    checkErrorCode(stmt, code, "failed to execute taos_stmt_set_tbname\n");

    ROWST rowst = {ts1, status};
    TAOS_BIND values[2];
    values[0].buffer_type = TSDB_DATA_TYPE_TIMESTAMP;
    values[0].buffer_length = sizeof(rowst.ts);
    values[0].length = &values[0].buffer_length;
    values[0].is_null = NULL;

    values[1].buffer_type = TSDB_DATA_TYPE_INT;
    values[1].buffer_length = sizeof(rowst.status);
    values[1].length = &values[1].buffer_length;
    values[1].is_null = NULL;

    values[0].buffer = &rowst.ts;
    values[1].buffer = &rowst.status;

    code = taos_stmt_bind_param(stmt, values); // bind param
    checkErrorCode(stmt, code, "failed to execute taos_stmt_bind_param");
    code = taos_stmt_add_batch(stmt); // add batch
    checkErrorCode(stmt, code, "failed to execute taos_stmt_add_batch");

    code = taos_stmt_execute(stmt);
    checkErrorCode(stmt, code, "failed to execute taos_stmt_execute");
    int affectedRows = taos_stmt_affected_rows(stmt);
    //printf("successfully inserted %d rows\n", affectedRows);
    // close
    taos_stmt_close(stmt);

    */
}

int Pv2TD(TAOS * taos, ARCHIVE_ELEMENT data)
{
    TAOS_RES* result;

    /*
    char str[256];
    char* dbrstr = dbr2str (data.data, data.type);
    //printf("data.type:%ld\n", data.type);
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
    */

    
    epicsTimeStamp ets = dbr2ts(data.data, data.type);//ets.secPastEpoch和ets.nsec均是uint类型
    char *status = dbr2status(data.data, data.type);
    char *severity = dbr2sev(data.data, data.type);
    char* value = val2str (data.data, data.type,0);
    
    /*
    switch (data.type)
    {
    case :
        
        break;
    
    default:
        break;
    }
    */

    //printf("dbr2st:%s\n", dbr2status(data.data, data.type));
    //printf("dbr2sev:%s\n", severity);
    //printf("ets.secPastEpoch:%u,ets.nsec:%d\n", ets.secPastEpoch, ets.nsec);

    if(ets.secPastEpoch != 0) {
        TAOS_STMT *stmt = taos_stmt_init(taos);
        const char *sql = "insert into ? using pv tags(0) values (?, ?, ?, ?);";
        //const char *sql = "insert into ? using pv tags(0) values (?, ?, ?, ?);";
        int code  = taos_stmt_prepare(stmt,sql, 0);//0表示会自动判断sql语句的长度
        checkErrorCode(stmt, code, "failed to excute taos_stmt_prepare\n");

        unsigned long ts1 = ets.secPastEpoch;//uint类型 * 1000会溢出，先转为ulong型
        unsigned long ts2 = ets.nsec;
        //secPastEpoch时间跟unix时间差了1970到1990的这2年，即7305 * 24 * 60 * 60 s 
        //tdengine需要的时间戳以毫秒为单位的时间戳
        ts1 = (ts1  + 631152000) * 1000 + ts2 / 1000000;
        
        char tbname[64];
        sprintf(tbname, "`%s`", data.pvname);
        code = taos_select_db(taos, "pvs");
        if (code != 0) {
            //database not exist
            printf("Database not specified or available\n");
            result = taos_query(Archiver->taos, "create database if not exists pvs;");
            taos_free_result(result);
            printf("Database pvs created!\n");
            result = taos_query(Archiver->taos, "use pvs;");
            taos_free_result(result);
            printf("Using database pvs...\n");
            result = taos_query(Archiver->taos, "create stable if not exists pv(ts TIMESTAMP, val FLOAT, status NCHAR(10), severity NCHAR(10)) tags(groupId INT);");
            taos_free_result(result);
            printf("Stable pv created!\n");
        }
        //printf("tbname: %s\n", tbname);
        code = taos_stmt_set_tbname(stmt, tbname);
        checkErrorCode(stmt, code, "failed to execute taos_stmt_set_tbname\n");

        /*
        ROW rowtest = {ts1, strtol(value, NULL, 10)};
        TAOS_BIND values[2];
        values[0].buffer_type = TSDB_DATA_TYPE_TIMESTAMP;
        values[0].buffer_length = sizeof(char*);
        values[0].length = &values[0].buffer_length;
        values[0].is_null = NULL;

        values[1].buffer_type = TSDB_DATA_TYPE_INT;
        values[1].buffer_length = sizeof(int);
        values[1].length = &values[1].buffer_length;
        values[1].is_null = NULL;

        values[0].buffer = &rowtest.ts;
        values[1].buffer = &rowtest.val;
        */

        ROWPV rowpv = {ts1, strtol(value, NULL, 10), status, severity};

        
        TAOS_BIND values[4];
        values[0].buffer_type = TSDB_DATA_TYPE_TIMESTAMP;
        values[0].buffer_length = sizeof(int64_t);
        values[0].length = &values[0].buffer_length;
        values[0].is_null = NULL;

        values[1].buffer_type = TSDB_DATA_TYPE_FLOAT;
        values[1].buffer_length = sizeof(rowpv.val);
        values[1].length = &values[1].buffer_length;
        values[1].is_null = NULL;

        values[2].buffer_type = TSDB_DATA_TYPE_NCHAR;
        values[2].buffer_length = sizeof(rowpv.status);
        values[2].length = &values[2].buffer_length;
        values[2].is_null = NULL;

        values[3].buffer_type = TSDB_DATA_TYPE_NCHAR;
        values[3].buffer_length = sizeof(rowpv.serverity);
        values[3].length = &values[3].buffer_length;
        values[3].is_null = NULL;

        values[0].buffer = &rowpv.ts;
        values[1].buffer = &rowpv.val;
        values[2].buffer = rowpv.status;//注意，status本身就是指针类型
        values[3].buffer = rowpv.serverity;
        
        code = taos_stmt_bind_param(stmt, values); // bind param
        checkErrorCode(stmt, code, "failed to execute taos_stmt_bind_param");
        code = taos_stmt_add_batch(stmt); // add batch
        checkErrorCode(stmt, code, "failed to execute taos_stmt_add_batch");

        code = taos_stmt_execute(stmt);
        checkErrorCode(stmt, code, "failed to execute taos_stmt_execute");
        //int affectedRows = taos_stmt_affected_rows(stmt);
        //printf("successfully inserted %d rows\n", affectedRows);
        // close
        taos_stmt_close(stmt);
    }


    /*
    //printf(dbr2str (data.data, data.type));
    //准备sql语句并往tdengine里写入数据，sql1插入数值变化，sql2插入状态变化
    char sql[256];
    //char sql2[256];
    char* errstr;

    
    
    sprintf(sql, "insert into pvs.`%s` using pvs.pv tags(0) values (\'%s\', \'%s\', \'%s\', \'%s\'); \n ", data.pvname, ts, value, status, severity);
    
    result = taos_query(Archiver->taos, sql);
    
    int errno = taos_errno(result);
    //printf("errno:%d", errno);
    if (result == NULL || errno != 0) {//如果taos_errno返回0说明执行成功
        printf("failed to insert row: %s, reason: %s\n", sql, taos_errstr(result));
        syslog(LOG_USER|LOG_INFO,"TDengine insert error\n");
        taos_free_result(result);

        if(errno == -2147482752) {//"Database not specified or available"，建库并且建超级表，之后再执行一遍插入
            printf("Database not specified or available\n");
            result = taos_query(Archiver->taos, "create database if not exists pvs;");
            taos_free_result(result);
            printf("Database pvs created!\n");
            result = taos_query(Archiver->taos, "use pvs;");
            taos_free_result(result);
            printf("Using database pvs...\n");
            result = taos_query(Archiver->taos, "create stable if not exists pv(ts TIMESTAMP, val FLOAT, status NCHAR(20), severity NCHAR(20)) tags(groupId INT);");
            taos_free_result(result);
            printf("Stable pv created!\n");
            sprintf(sql, "insert into pvs.`%s` using pvs.pv tags(0) values (\'%s\', \'%s\', \'%s\', \'%s\'); \n ", data.pvname, ts, value, status, severity);
            result = taos_query(Archiver->taos, sql);
            errno = taos_errno(result);
        } 
        if(errno == -2147482782) {//"Table does not exist"，有库没表，建超级表，之后再执行一次插入
            printf("Table does not exist\n");
            result = taos_query(Archiver->taos, "use pvs;");
            taos_free_result(result);
            printf("Using database pvs...\n");
            result = taos_query(Archiver->taos, "create stable if not exists pv(ts TIMESTAMP, val FLOAT, status NCHAR(20), severity NCHAR(20)) tags(groupId INT);");
            taos_free_result(result);
            printf("Stable pv created!\n");
            sprintf(sql, "insert into pvs.`%s` using pvs.pv tags(0) values (\'%s\', \'%s\', \'%s\', \'%s\'); \n ", data.pvname, ts, value, status, severity);
            result = taos_query(Archiver->taos, sql);
            errno = taos_errno(result);
        }
        
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
         
CREATE TABLE IF NOT EXISTS monitor_pv(ts TIMESTAMP, callbackcounts INT, npvon  INT, npvoff INT);

    } else {
        #ifdef DEBUG
        printf("insert row: %s result: success\n", sql);
        #endif
    }
    taos_free_result(result);
    */
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
    int errno = taos_errno(result);
    if (result == NULL || errno != 0) {//如果taos_errno返回0说明执行成功
        printf("failed to insert row: %s, reason: %s\n", sql, taos_errstr(result));
        syslog(LOG_USER|LOG_INFO,"TDengine insert error\n");
        taos_free_result(result);

    if(errno == -2147482752) {//"Database not specified or available"，建库并且建超级表，之后再执行一遍插入
            printf("Database not specified or available\n");
            result = taos_query(Archiver->taos, "create database if not exists monitor;");
            taos_free_result(result);
            printf("Database monitor created!\n");
            result = taos_query(Archiver->taos, "use monitor;");
            taos_free_result(result);
            printf("Using database monitor...\n");
            result = taos_query(Archiver->taos, "CREATE TABLE IF NOT EXISTS monitor_pv(ts TIMESTAMP, callbackcounts INT, npvon  INT, npvoff INT);");
            taos_free_result(result);
            printf("Table monitor_pv created!\n");
            result = taos_query(Archiver->taos, "CREATE TABLE IF NOT EXISTS monitor(ts TIMESTAMP, status INT);");
            taos_free_result(result);
            sprintf(sql, "insert into monitor.monitor_pv values (\'%s\', %d, %d, %d) \n" , timeText, callBackCounts, nPvOn, nPvOff);
            result = taos_query(Archiver->taos, sql);
            errno = taos_errno(result);
        } 
        if(errno == -2147482782) {//"Table does not exist"，有库没表，建超级表，之后再执行一次插入
            printf("Table does not exist\n");
            result = taos_query(Archiver->taos, "use monitor;");
            taos_free_result(result);
            printf("Using database monitor...\n");
            result = taos_query(Archiver->taos, "CREATE TABLE IF NOT EXISTS monitor_pv(ts TIMESTAMP, callbackcounts BIGINT, npvon  INT, npvoff INT);");
            taos_free_result(result);
            printf("Table monitor_pv created!\n");
            sprintf(sql, "insert into monitor.monitor_pv values (\'%s\', %d, %d, %d) \n" , timeText, callBackCounts, nPvOn, nPvOff);
            result = taos_query(Archiver->taos, sql);
            errno = taos_errno(result);
        }

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
    /*
    TAOS_RES* result;
    epicsTimeStamp tsNow;
    epicsTimeGetCurrent(&tsNow);
    TAOS_STMT *stmt = taos_stmt_init(taos);
    int code = taos_select_db(taos, "monitor");
    //printf("HB2TD_error_code: %d", code);
    if (code != 0) {
        //database not exist
        printf("Database not specified or available\n");
        result = taos_query(Archiver->taos, "create database if not exists monitor;");
        taos_free_result(result);
        printf("Database monitor created!\n");
        result = taos_query(Archiver->taos, "use monitor;");
        taos_free_result(result);
        printf("Using database monitor...\n");
        result = taos_query(Archiver->taos, "CREATE TABLE IF NOT EXISTS monitor_pv(ts TIMESTAMP, callbackcounts INT, npvon  INT, npvoff INT);");
        taos_free_result(result);
        printf("Table monitor_pv created!\n");
    }
    const char *sql = "insert into monitor_pv values (?, ?, ?, ?);";
    //const char *sql = "insert into ? using pv tags(0) values (?, ?, ?, ?);";
    code  = taos_stmt_prepare(stmt,sql, 0);//0表示会自动判断sql语句的长度
    checkErrorCode(stmt, code, "failed to excute taos_stmt_prepare\n");

    unsigned long ts1 = tsNow.secPastEpoch;//uint类型 * 1000会溢出，先转为ulong型
    unsigned long ts2 = tsNow.nsec;
    //secPastEpoch时间跟unix时间差了1970到1990的这2年，即7305 * 24 * 60 * 60 s 
    //tdengine需要的时间戳以毫秒为单位的时间戳
    ts1 = (ts1  + 631152000) * 1000 + ts2 / 1000000;
    //printf("HB2TDts1: %lu\n", ts1);

    

    ROWMPV rowmpv = {ts1, callBackCounts, nPvOn, nPvOff};

    TAOS_BIND values[4];
    values[0].buffer_type = TSDB_DATA_TYPE_TIMESTAMP;
    values[0].buffer_length = sizeof(int64_t);
    values[0].length = &values[0].buffer_length;
    values[0].is_null = NULL;

    values[1].buffer_type = TSDB_DATA_TYPE_INT;
    values[1].buffer_length = sizeof(int);
    values[1].length = &values[1].buffer_length;
    values[1].is_null = NULL;

    values[2].buffer_type = TSDB_DATA_TYPE_INT;
    values[2].buffer_length = sizeof(int);
    values[2].length = &values[2].buffer_length;
    values[2].is_null = NULL;

    values[3].buffer_type = TSDB_DATA_TYPE_INT;
    values[3].buffer_length = sizeof(int);
    values[3].length = &values[3].buffer_length;
    values[3].is_null = NULL;

    values[0].buffer = &rowmpv.ts;
    values[1].buffer = &rowmpv.callbackcounts;
    values[2].buffer = &rowmpv.npvon;
    values[3].buffer = &rowmpv.npvoff;

    code = taos_stmt_bind_param(stmt, values); // bind param
    checkErrorCode(stmt, code, "failed to execute taos_stmt_bind_param");
    code = taos_stmt_add_batch(stmt); // add batch
    checkErrorCode(stmt, code, "failed to execute taos_stmt_add_batch");

    code = taos_stmt_execute(stmt);
    checkErrorCode(stmt, code, "failed to execute taos_stmt_execute");
    //int affectedRows = taos_stmt_affected_rows(stmt);
    //printf("successfully inserted %d rows\n", affectedRows);
    // close
    taos_stmt_close(stmt);
    */    
}

/**
 * @brief check return status and exit program when error occur.
 * 
 * @param stmt 
 * @param code 
 * @param msg 
 */
void checkErrorCode(TAOS_STMT *stmt, int code, const char* msg) {
    if (code != 0) {
        printf("code: %d\n", code);
        printf("%s. error: %s\n", msg, taos_stmt_errstr(stmt));
        //taos_stmt_close(stmt);
        //exit(EXIT_FAILURE);
    }
}