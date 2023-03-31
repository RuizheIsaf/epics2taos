
#include"loadConfig.h"
 
 
//获得文件的有效行数
int getLines_configFile(FILE* file) {
	char buf[1024] = { 0 };
	int lines = 0;
	while (fgets(buf, 1024, file) != NULL) {
		if (!isValid_configFile(buf)) {
			continue;
		}
 
		++lines;
		memset(buf, 0, 1024);
	}
	//把文件指针重置到文件开头
	fseek(file, 0, SEEK_SET);
	return lines;
}
 
//加在配置文件
void loadFile_configFile(const char* filePath, char*** fileData, int* lines) {
	FILE* file = fopen(filePath, "r");
	if (NULL == file) {
		return;
	}
 
	int line = getLines_configFile(file);
	//给每行数据开辟内存
	char **temp = malloc(sizeof(char *) *line);
 
	char buf[1024] = {0};
	int index = 0;
	while (fgets(buf, 1024, file) != NULL) {
 
		//如果该行无效则跳过
		if (!isValid_configFile(buf)) {
			continue;
		}
 
		//给每一行分配内存空间并赋值
		temp[index] = malloc(strlen(buf) + 1);
		strcpy(temp[index], buf);
		//清空buf
		memset(buf, 0, 1024);
		++index;
 
	}
 
	*fileData = temp;
	*lines = line;
 
 
}
 
//解析配置文件
void parseFile_configFile(char  **fileData, int lines, struct ConfigInfo** info) {
	struct ConfigInfo* myinfo = malloc(sizeof(struct ConfigInfo) * lines);
	memset(myinfo, 0, sizeof(struct ConfigInfo) * lines);
    int i;
	for (i = 0; i < lines; ++i) {
		char* pos = strchr(fileData[i],'@');
		//printf("---------------%s\n", pos);
		strncpy(myinfo[i].key, fileData[i], pos - fileData[i]);
		strncpy(myinfo[i].value, pos+1, strlen(pos + 1) -1);
		#ifdef DEBUG
		printf("-------configInfo[%d].key--------%s\n", i, myinfo[i].key);
		printf("-------configInfo[%d].value--------%s\n", i, myinfo[i].value);
		#endif
	}
	//释放fileData
    int j;
	for (j = 0; j < lines; ++j)
	{
		if (fileData[j] != NULL) {
			free(fileData[j]);
			fileData[j] = NULL;
		}
	}
	*info = myinfo;
}
 
//获取指定的配置信息
char* getInfo_configFile(const char* key, struct ConfigInfo* info,int lines) {
	int i;
    for (i = 0; i < lines; ++i) {
		if (strcmp(key,info[i].key) == 0) {
			return info[i].value;
		}
	}
	return NULL;
}
 
//释放配置文件信息
void destryInfo_configFile(struct ConfigInfo* info) {
	if (NULL == info) {
		return;
	}
	free(info);
	info = NULL;
}
 
//判断当前行是否有效
int isValid_configFile(const char* buf) {
	if (buf[0] == '#' || buf[0] == '\n' || strchr(buf, '@') == NULL) {
		return 0;
	}
 
	return 1;
}
