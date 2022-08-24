
#define _CRT_SECURE_NO_WARNINGS
#pragma once
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
 
struct ConfigInfo {
	char key[64];
	char value[128];
};
 
//为了在C++中能够调用C的函数
#ifdef __cplusplus
extern "C" {
#endif
	//获得文件的有效行数
	int getLines_configFile(FILE  *file);
 
	//加在配置文件
	void loadFile_configFile(const char* filePath,char ***fileData,int *lines);
 
	//解析配置文件
	void parseFile_configFile(char **fileData,int lines,struct ConfigInfo **info);
 
	//获取指定的配置信息
	char* getInfo_configFile(const char *key,struct ConfigInfo *info,int lines);
 
	//释放配置文件信息
	void destryInfo_configFile(struct ConfigInfo *info);
 
	//判断当前行是否有效
	int isValid_configFile(const char *buf);
 
#ifdef __cplusplus
}

#endif
