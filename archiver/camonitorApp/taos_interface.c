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
    char *host = getInfo_configFile("taos_host", info, lines);
    char *user = getInfo_configFile("taos_user", info, lines);
    char *passwd = getInfo_configFile("taos_passwd", info, lines);
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

    sprintf(sql, "insert into status.`%s` using status.st tags(0) values (\'%s\', %d) \n" , ppv->name, timeText, status);
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
}

int Pv2TD(TAOS * taos, ARCHIVE_ELEMENT data)
{
    TAOS_RES* result;
    int errno;    
    epicsTimeStamp ets = dbr2ts(data.data, data.type);//ets.。secPastEpoch和ets.nsec均是uint类型
    char *status = dbr2status(data.data, data.type);
    char *severity = dbr2sev(data.data, data.type);
    char* value = val2str(data.data, data.type,0);
    
    unsigned base_type;
    base_type = data.type % (LAST_TYPE+1);

    if (data.type == DBR_STSACK_STRING || data.type == DBR_CLASS_NAME)
        base_type = DBR_STRING;
    
    char *valstr;
    char valch;
    float valf;
    double vald;
    int vali;
    long vall;
    char sql[256];
    char *sql1;
    
    if(ets.secPastEpoch != 0) {
        unsigned long ts1 = epicsTime2int(ets);
        switch (base_type){
        case DBR_STRING:
            valstr = val_str(data.data, data.type, 0);
            sprintf(sql, "insert into pvs.`%s` using pvs.pvstr tags(0) values (%lu, \'%s\', \'%s\', \'%s\'); \n ", data.pvname, ts1, valstr, status, severity);
            result = taos_query(Archiver->taos, sql);
            errno = taos_errno(result);
            if(result == NULL || errno != 0) {
                printf("failed to insert row: %s, reason: %s\n", sql, taos_errstr(result));
                syslog(LOG_USER|LOG_INFO,"TDengine insert error\n");
                taos_free_result(result);
                sql1 = "create stable if not exists pvstr(ts TIMESTAMP, val NCHAR(20), status NCHAR(20), severity NCHAR(20)) tags(groupId INT);";
                checkResult(errno, sql1, sql); 
            }
            //printf("sql:%s\n", sql);
            //printf("valstr:%s\n", valstr);
            taos_free_result(result);
            break;
        case DBR_FLOAT:
            valf = val_float(data.data, data.type, 0);
            sprintf(sql, "insert into pvs.`%s` using pvs.pvf tags(0) values (%lu, \'%f\', \'%s\', \'%s\'); \n ", data.pvname, ts1, valf, status, severity);
            result = taos_query(Archiver->taos, sql);
            errno = taos_errno(result);
            if(result == NULL || errno != 0) {
                printf("failed to insert row: %s, reason: %s\n", sql, taos_errstr(result));
                syslog(LOG_USER|LOG_INFO,"TDengine insert error\n");
                taos_free_result(result);
                sql1 = "create stable if not exists pvf(ts TIMESTAMP, val FLOAT, status NCHAR(20), severity NCHAR(20)) tags(groupId INT);";
                checkResult(errno, sql1, sql);
            }
            //printf("sql:%s\n", sql);
            //printf("valfloat:%f\n", valf);
            taos_free_result(result);
            break;
        case DBR_DOUBLE:
            vald = val_double(data.data, data.type, 0);
            sprintf(sql, "insert into pvs.`%s` using pvs.pvd tags(0) values (%lu, \'%f\', \'%s\', \'%s\'); \n ", data.pvname, ts1, vald, status, severity);
            result = taos_query(Archiver->taos, sql);
            errno = taos_errno(result);
            if(result == NULL || errno != 0) {
                printf("failed to insert row: %s, reason: %s\n", sql, taos_errstr(result));
                syslog(LOG_USER|LOG_INFO,"TDengine insert error\n");
                taos_free_result(result);
                sql1 = "create stable if not exists pvd(ts TIMESTAMP, val DOUBLE, status NCHAR(20), severity NCHAR(20)) tags(groupId INT);";
                checkResult(errno, sql1, sql);
            }
            //printf("sql:%s\n", sql);
            //printf("valdouble:%f\n", vald);
            taos_free_result(result);
            break;
        case DBR_CHAR:
            valch = val_char(data.data, data.type, 0);
            sprintf(sql, "insert into pvs.`%s` using pvs.pvch tags(0) values (%lu, \'%s\', \'%s\', \'%s\'); \n ", data.pvname, ts1, valch, status, severity);
            result = taos_query(Archiver->taos, sql);
            errno = taos_errno(result);
            if(result == NULL || errno != 0) {
                printf("failed to insert row: %s, reason: %s\n", sql, taos_errstr(result));
                syslog(LOG_USER|LOG_INFO,"TDengine insert error\n");
                taos_free_result(result);
                sql1 = "create stable if not exists pvch(ts TIMESTAMP, val BINARY, status NCHAR(20), severity NCHAR(20)) tags(groupId INT);";
                checkResult(errno, sql1, sql);
            }
            //printf("sql:%s\n", sql);
            //printf("valch:%s\n", valch);
            taos_free_result(result);
            break;
        case DBR_INT:   
            vali = val_int(data.data, data.type, 0);
            sprintf(sql, "insert into pvs.`%s` using pvs.pvi tags(0) values (%lu, \'%d\', \'%s\', \'%s\'); \n ", data.pvname, ts1, vali, status, severity);
            result = taos_query(Archiver->taos, sql);
            errno = taos_errno(result);
            if(result == NULL || errno != 0) {
                printf("failed to insert row: %s, reason: %s\n", sql, taos_errstr(result));
                syslog(LOG_USER|LOG_INFO,"TDengine insert error\n");
                taos_free_result(result);
                sql1 = "create stable if not exists pvi(ts TIMESTAMP, val INT, status NCHAR(20), severity NCHAR(20)) tags(groupId INT);";
                checkResult(errno, sql1, sql);
            }
            //printf("sql:%s\n", sql);
            //printf("vaint:%d\n", vali);
            taos_free_result(result);
            break;
        case DBR_LONG:
            vall = val_long(data.data, data.type, 0);
            sprintf(sql, "insert into pvs.`%s` using pvs.pvl tags(0) values (%lu, \'%ld\', \'%s\', \'%s\'); \n ", data.pvname, ts1, vall, status, severity);
            result = taos_query(Archiver->taos, sql);
            errno = taos_errno(result);
            if(result == NULL || errno != 0) {
                printf("failed to insert row: %s, reason: %s\n", sql, taos_errstr(result));
                syslog(LOG_USER|LOG_INFO,"TDengine insert error\n");
                taos_free_result(result);
                sql1 = "create stable if not exists pvl(ts TIMESTAMP, val INT, status NCHAR(20), severity NCHAR(20)) tags(groupId INT);";
                checkResult(errno, sql1, sql);
            }
            //printf("sql:%s\n", sql);
            //printf("vallong:%ld\n", vall);
            taos_free_result(result);
            break;
        default:
            break;
        }


    }
    
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

void checkResult(int errno, char* sql1, char* sql2) {
    TAOS_RES *result;
    char sql[265];
    if(errno == -2147482752) {//"Database not specified or available"，建库并且建超级表，之后再执行一遍插入
        printf("Database not specified or available\n");
        result = taos_query(Archiver->taos, "create database if not exists pvs precision 'ns';");
        taos_free_result(result);
        printf("Database pvs created!\n");
        result = taos_query(Archiver->taos, "use pvs;");
        taos_free_result(result);
        printf("Using database pvs...\n");
        result = taos_query(Archiver->taos, sql1);
        taos_free_result(result);
        printf("Stable created!\n");
        result = taos_query(Archiver->taos, sql2);
        errno = taos_errno(result);
    } 
    if(errno == -2147482782) {//"Table does not exist"，有库没表，建超级表，之后再执行一次插入
        printf("Table does not exist\n");
        result = taos_query(Archiver->taos, "use pvs;");
        taos_free_result(result);
        printf("Using database pvs...\n");
        result = taos_query(Archiver->taos, sql1);
        taos_free_result(result);
        printf("Stable created!\n");
        result = taos_query(Archiver->taos, sql2);
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

}

void executeSQL(TAOS *taos, const char *sql) {
  TAOS_RES *res = taos_query(taos, sql);
  int       code = taos_errno(res);
  if (code != 0) {
    printf("%s\n", taos_errstr(res));
    taos_free_result(res);
    taos_close(taos);
    //exit(EXIT_FAILURE);
  }
  taos_free_result(res);
}

//EPICS的字符串是40个字节，但是惟斌建立的表字符串长度设置成20。。。这个长度似乎不能改，只能和表的长度一致。
#define NATIVE_DBR_TO_BIND(EPICS_TYPE,TAOS_TYPE)                                             \
    values[0].buffer_type = TSDB_DATA_TYPE_TIMESTAMP;                                        \
    values[0].buffer_length = sizeof(uint64_t);                                               \
    values[0].length = &values[0].buffer_length;                                             \
    values[0].is_null = NULL;                                                                \
                                                                                             \
    values[1].buffer_type = TAOS_TYPE;                                                       \   
    if (values[1].buffer_type == TSDB_DATA_TYPE_NCHAR)                                       \
    {values[1].buffer_length =20;}                                                           \
    else                                                                                     \
    { values[1].buffer_length = sizeof(((struct EPICS_TYPE*)(&data.data))->value);}          \      
    values[1].length = &values[1].buffer_length;                                             \
    values[1].is_null = NULL;                                                                \
                                                                                             \
    values[2].buffer_type = TSDB_DATA_TYPE_NCHAR;                                            \
    values[2].buffer_length = strlen(status);                                                \             
    values[2].length = &values[2].buffer_length;                                             \
    values[2].is_null = NULL;                                                                \
                                                                                             \
    values[3].buffer_type = TSDB_DATA_TYPE_NCHAR;                                            \
    values[3].buffer_length = strlen(severity);                                              \             
    values[3].length = &values[3].buffer_length;                                             \
    values[3].is_null = NULL;                                                                \
                                                                                             \
    values[0].buffer = &taos_ts;                                                             \
    values[1].buffer = &(((struct EPICS_TYPE*)(&data.data))->value);                          \                 
    values[2].buffer = status;                                                               \               
    values[3].buffer = severity;                                                             \
   


epicsUInt16 dbr2taosbind(TAOS_BIND *values, ARCHIVE_ELEMENT data)
{
    epicsTimeStamp ts;
    ts = dbr2ts(data.data, data.type);
    uint64_t taos_ts = epicsTime2int(ts);
                                                                                           
    char *status = dbr2status(data.data, data.type);                                         
    char *severity = dbr2sev(data.data, data.type);                                          
    
    
    switch (data.type)
    {
    case DBR_TIME_STRING:     
        NATIVE_DBR_TO_BIND(dbr_time_string, TSDB_DATA_TYPE_NCHAR);
        break;
    case DBR_TIME_SHORT:
        NATIVE_DBR_TO_BIND(dbr_time_short, TSDB_DATA_TYPE_SMALLINT); 
        break;
    case DBR_TIME_FLOAT:
        NATIVE_DBR_TO_BIND(dbr_time_float, TSDB_DATA_TYPE_FLOAT); 
        break;
    case DBR_TIME_ENUM:
        NATIVE_DBR_TO_BIND(dbr_time_enum,TSDB_DATA_TYPE_USMALLINT); 
        break;
    case DBR_TIME_CHAR:
        NATIVE_DBR_TO_BIND(dbr_time_char,TSDB_DATA_TYPE_UINT); 
        break;
    case DBR_TIME_LONG:
        NATIVE_DBR_TO_BIND(dbr_time_long,TSDB_DATA_TYPE_INT); 
        break;
    case DBR_TIME_DOUBLE:
        NATIVE_DBR_TO_BIND(dbr_time_double,TSDB_DATA_TYPE_DOUBLE);
        break;
    default: 
        printf("can't reconize data type\n");   
        return 0;
    }
    return 1;
}

#define SQL_STR     "insert into ? using pvstr tags(0) values (?, ?, ?, ?);"
#define SQL_FLOAT   "insert into ? using pvf tags(0) values (?, ?, ?, ?);"
#define SQL_DOUBLE  "insert into ? using pvd tags(0) values (?, ?, ?, ?);"
#define SQL_CHAR    "insert into ? using pvch tags(0) values (?, ?, ?, ?);"
#define SQL_INT     "insert into ? using pvi tags(0) values (?, ?, ?, ?);"
#define SQL_LONG    "insert into ? using pvl tags(0) values (?, ?, ?, ?);"

#define SQL_CREATE_STR    "create stable if not exists pvstr(ts TIMESTAMP, val NCHAR(20), status NCHAR(20), severity NCHAR(20)) tags(groupId INT);"
#define SQL_CREATE_FLOAT  "create stable if not exists pvf(ts TIMESTAMP, val FLOAT, status NCHAR(20), severity NCHAR(20)) tags(groupId INT);"
#define SQL_CREATE_DOUBLE "create stable if not exists pvd(ts TIMESTAMP, val DOUBLE, status NCHAR(20), severity NCHAR(20)) tags(groupId INT);"
#define SQL_CREATE_CHAR   "create stable if not exists pvch(ts TIMESTAMP, val BINARY(1), status NCHAR(20), severity NCHAR(20)) tags(groupId INT);"
#define SQL_CREATE_INT    "create stable if not exists pvi(ts TIMESTAMP, val INT, status NCHAR(20), severity NCHAR(20)) tags(groupId INT);"
#define SQL_CREATE_LONG   "create stable if not exists pvl(ts TIMESTAMP, val INT, status NCHAR(20), severity NCHAR(20)) tags(groupId INT);"


int Pv2TD_bind(TAOS * taos,ARCHIVE_ELEMENT data)
{
    static epicsBoolean stmt_initialized = false;     //静态变量，去查静态变量的意思
    static TAOS_STMT * stmt[30];
    static int code;
    static epicsUInt16  batch_count[30];
    
    /*
    if (data.type == 19 )              //表建立的不对。。EPICS的long长度为4字节，表里设置成了8字节
    {
        return;
    }
    */

    TAOS_RES* result; 
    //判断是否已经初始化了stmt，已经初始化了就不再重复执行，只需调用已经有的stmt就行
    if (!stmt_initialized)
    {
        //size_t i;
        code = taos_select_db(taos, "pvs");
        if (code != 0) {
            //database not exist
            printf("Database not specified or available\n");
            result = taos_query(Archiver->taos, "create database if not exists pvs precision 'ns';");
            taos_free_result(result);
            printf("Database pvs created!\n");
            result = taos_query(Archiver->taos, "use pvs;");
            taos_free_result(result);
            printf("Using database status...\n");
        }

        result = taos_query(Archiver->taos, SQL_CREATE_STR);
        taos_free_result(result);
        result = taos_query(Archiver->taos, SQL_CREATE_FLOAT);
        taos_free_result(result);
        result = taos_query(Archiver->taos, SQL_CREATE_DOUBLE);
        taos_free_result(result);
        result = taos_query(Archiver->taos, SQL_CREATE_CHAR);
        taos_free_result(result);
        result = taos_query(Archiver->taos, SQL_CREATE_INT);
        taos_free_result(result);
        result = taos_query(Archiver->taos, SQL_CREATE_LONG);
        taos_free_result(result);

        //stmt是个指针数组，长度是30，但实际上只用到了14，15，16，18，19，20这六个
        //实际上只初始化了数组的这六个位置，分别准备了不同类型的stmt地址存进去，之后根据来的data.type不同使用不同的stmt
        stmt[DBR_TIME_STRING] = taos_stmt_init(taos);  
        stmt[DBR_TIME_FLOAT] = taos_stmt_init(taos);  
        stmt[DBR_TIME_DOUBLE] = taos_stmt_init(taos);  
        stmt[DBR_TIME_CHAR] = taos_stmt_init(taos);  
        stmt[DBR_TIME_INT] = taos_stmt_init(taos);  
        stmt[DBR_TIME_LONG] = taos_stmt_init(taos); 

        code  = taos_stmt_prepare(stmt[DBR_TIME_STRING],SQL_STR, 0);     //0表示会自动判断sql语句的长度
        checkErrorCode(stmt[DBR_TIME_STRING], code, "failed to excute taos_stmt_prepare\n");
        //stmt[DBR_TIME_FLOAT] = taos_stmt_init(taos);  
        code  = taos_stmt_prepare(stmt[DBR_TIME_FLOAT],SQL_FLOAT, 0);     
        checkErrorCode(stmt[DBR_TIME_FLOAT], code, "failed to excute taos_stmt_prepare\n");
        code  = taos_stmt_prepare(stmt[DBR_TIME_DOUBLE],SQL_DOUBLE, 0);     
        checkErrorCode(stmt[DBR_TIME_DOUBLE], code, "failed to excute taos_stmt_prepare\n");
        code  = taos_stmt_prepare(stmt[DBR_TIME_CHAR],SQL_CHAR, 0);     
        checkErrorCode(stmt[DBR_TIME_CHAR], code, "failed to excute taos_stmt_prepare\n");
        code  = taos_stmt_prepare(stmt[DBR_TIME_INT],SQL_INT, 0);     
        checkErrorCode(stmt[DBR_TIME_INT], code, "failed to excute taos_stmt_prepare\n");
        code  = taos_stmt_prepare(stmt[DBR_TIME_LONG],SQL_LONG, 0);     
        checkErrorCode(stmt[DBR_TIME_LONG], code, "failed to excute taos_stmt_prepare\n");
        stmt_initialized = true;//初始化完毕，将该静态变量设为真，以后就不会再重复初始化stmt
    }
    
    char tbname[64];

    sprintf(tbname, "`%s`", data.pvname);
    //printf("archiving pv: %s\n",tbname);
    code = taos_stmt_set_tbname(stmt[data.type], tbname);
    checkErrorCode(stmt[data.type], code, "failed to execute taos_stmt_set_tbname\n");

    TAOS_BIND values[4];
    dbr2taosbind(&values,data);

    code = taos_stmt_bind_param(stmt[data.type], values); // bind param
    if (code == 0)//code = 0 时bind没有报错
    {   
        code = taos_stmt_add_batch(stmt[data.type]); // add batch
        checkErrorCode(stmt[data.type], code, "failed to execute taos_stmt_add_batch");
        batch_count[data.type] = batch_count[data.type] + 1;//绑定没问题的话计数加1
    }
    else//错误处理
    {
        printf("----------------------\n");
        checkErrorCode(stmt[data.type], code, "failed to execute taos_stmt_bind_param");
        printf("archiving pv: %s\n",data.pvname);
        printf("type is %lu\n",data.type);
        printf("----------------------\n");
    }
        
    if (batch_count[data.type] > 50)//同类型数据绑定超过50条时执行一次taos_stmt_execute
    {
        //printf("archiving pv: %s\n",data.pvname);
        //printf("type is %lu\n",data.type);
        code = taos_stmt_execute(stmt[data.type]);  
        if (code != 0)
        {
            printf("----------------------\n");
            checkErrorCode(stmt[data.type], code, "failed to execute taos_stmt_execute");
            printf("archiving pv: %s\n",data.pvname);
            printf("type is %lu\n",data.type);
            printf("----------------------\n");
        }
        
        batch_count[data.type] = 0;//插入完计数重新归零
    }
    
    //taos_stmt_close(stmt[data.type]);
}

void PvArray2TD(TAOS * taos, unsigned long ts, char* pvname, long type, long count, char* status, char* sev)
{
    TAOS_RES* result;
    char sql[256];
    char* errstr;
    sprintf(sql, "insert into s3ref.`%s` using s3ref.pvref tags(0) values (%lu, %lu, %lu, \'%s\', \'%s\'); \n ", pvname, ts, type, count, status, sev);

    
    result = taos_query(Archiver->taos, sql);
    int errno = taos_errno(result);
    if (result == NULL || errno != 0) {//如果taos_errno返回0说明执行成功
        printf("failed to insert row: %s, reason: %s\n", sql, taos_errstr(result));
        syslog(LOG_USER|LOG_INFO,"TDengine insert error\n");
        taos_free_result(result);
        if(errno == -2147482752) {//"Database not specified or available"，建库并且建超级表，之后再执行一遍插入
            printf("Database not specified or available\n");
            result = taos_query(Archiver->taos, "create database if not exists s3ref precision 'ns';");
            taos_free_result(result);
            printf("Database s3ref created!\n");
            result = taos_query(Archiver->taos, "use s3ref;");
            taos_free_result(result);
            printf("Using database s3ref...\n");
            result = taos_query(Archiver->taos, "create stable if not exists pvref(ts TIMESTAMP, type BIGINT, count BIGINT, status NCHAR(20), severity NCHAR(20)) tags(groupId INT);");
            taos_free_result(result);
            printf("Stable pvref created!\n");
            result = taos_query(Archiver->taos, "use pvs;");
            taos_free_result(result);
            result = taos_query(Archiver->taos, sql);
            errno = taos_errno(result);
            printf("error:%d\n", errno);
            taos_free_result(result);
        } 
        if(errno == -2147482782) {//"Table does not exist"，有库没表，建超级表，之后再执行一次插入
            printf("Table does not exist\n");
            result = taos_query(Archiver->taos, "use s3ref;");
            taos_free_result(result);
            printf("Using database s3ref...\n");
            result = taos_query(Archiver->taos, "create stable if not exists pvref(ts TIMESTAMP, type BIGINT, count BIGINT, status NCHAR(20), severity NCHAR(20)) tags(groupId INT);");
            taos_free_result(result);
            printf("Stable pvref created!\n");
            result = taos_query(Archiver->taos, "use pvs;");
            taos_free_result(result);
            result = taos_query(Archiver->taos, sql);
            errno = taos_errno(result);
            printf("error:%d\n", errno);
            taos_free_result(result);
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
    taos_free_result(result);
}
