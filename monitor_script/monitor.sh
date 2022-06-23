#!/bin/sh
 
# 函数: CheckProcess
# 功能: 检查一个进程是否存在
# 参数: $1 --- 要检查的进程名称
# 返回: 如果存在返回0, 否则返回1.
#------------------------------------------------------------------------------
CheckProcess()
{
  # 检查输入的参数是否有效
  if [ "$1" = "" ];
  then
    return 1
  fi
 
  #$PROCESS_NUM获取指定进程名的数目，为1返回0，表示正常，不为1返回1，表示有错误，需要重新启动
  #'ps -ef' lists all the process
  #'grep' finds the input; 'grep -v' excludes the input
  #'wc -l' returns the number of lines in the result 
  PROCESS_NUM=`ps -ef | grep "$1" | grep -v "grep" | wc -l` 
  if [ $PROCESS_NUM -eq 1 ];
  then
    return 0
  else
    return 1
  fi
}
 
 
# 检查test实例是否已经存在
while [ 1 ] ; do 
     CheckProcess "caMonitor"
     #'$?' checks the result of the last command: 
     #"0" means the command was excuted successfully
     #"1" means the command was excuted with failure
     Check_RET=$?
     if [ $Check_RET -eq 1 ];
     then
        echo "caMonitor服务不正常"
        # 隐藏启动test 
        # nohup ./test  > nohuptest.file 2>&1 &
	# insert into tdengine	echo $(date +%s)
	# curl -H 'Authorization: Basic cm9vdDp0YW9zZGF0YQ==' -d 'insert into monitor values($(date +%s), 0);' 172.16.5.20:6041/rest/sql/monitor
	str1="insert into monitor values('"
	str2=$(date +"%Y-%m-%d %H:%M:%S")
	str3="', 0);"
	echo "$str1$str2$str3"
	curl -H 'Authorization: Basic cm9vdDp0YW9zZGF0YQ==' -d "$str1$str2$str3" localhost:6041/rest/sql/monitor
	echo " "
    else
        echo "caMonitor服务正常"
	# insert into tdengine
	str1="insert into monitor values('"
	str2=$(date +"%Y-%m-%d %H:%M:%S")
	str3="', 1);"
	echo "$str1$str2$str3"
	curl -H 'Authorization: Basic cm9vdDp0YW9zZGF0YQ==' -d "$str1$str2$str3" localhost:6041/rest/sql/monitor
	echo " "
    fi
 #check every 5 seconds
 sleep 5
done

