#!bin/bash

if [ $# -lt 2 ]
then
	echo "缺少参数"
	exit 1
fi

N=$1
LISTNAME=$2

if [ -f "$2" ];then
	rm "$2"
	touch "$2"
	echo "#pvlist" >> "$2"
else
	touch "$2"
	echo "#pvlist" >> "$2"
fi

for ((i=0;i<$N;i++))
do
	sed -i "` expr $i \* 18 + 1` a ioc$i:calcExample" "$2"
	sed -i "` expr $i \* 18 + 2 ` a ioc$i:calcExample1" "$2"
	sed -i "` expr $i \* 18 + 3 ` a ioc$i:calcExample2" "$2"
	sed -i "` expr $i \* 18 + 4 ` a ioc$i:calcExample3" "$2"
	sed -i "` expr $i \* 18 + 5 ` a ioc$i:calc1" "$2"
	sed -i "` expr $i \* 18 + 6 ` a ioc$i:calc2" "$2"
	sed -i "` expr $i \* 18 + 7 ` a ioc$i:calc3" "$2"
	sed -i "` expr $i \* 18 + 8 ` a ioc$i:aiExample" "$2"
	sed -i "` expr $i \* 18 + 9 ` a ioc$i:aiExample1" "$2"
	sed -i "` expr $i \* 18 + 10 ` a ioc$i:aiExample2" "$2"
	sed -i "` expr $i \* 18 + 11 ` a ioc$i:aiExampl3" "$2"
	sed -i "` expr $i \* 18 + 12 ` a ioc$i:ai1" "$2"
	sed -i "` expr $i \* 18 + 13 ` a ioc$i:ai2" "$2"
	sed -i "` expr $i \* 18 + 14 ` a ioc$i:ai3" "$2"
	sed -i "` expr $i \* 18 + 15 ` a ioc$i:xxxExample" "$2"
	sed -i "` expr $i \* 18 + 16 ` a ioc$i:compressExample" "$2"
	sed -i "` expr $i \* 18 + 17 ` a ioc$i:subExample" "$2"
	sed -i "` expr $i \* 18 + 18 ` a ioc$i:aSubExample" "$2"
done
