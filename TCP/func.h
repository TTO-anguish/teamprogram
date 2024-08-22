/**
 * @FilePath     : /home/ZF/TCP/func.h
 * @Description  :  
 * @Author       : zongfei
 * @Version      : 0.0.1
 * @LastEditors  : zongfei
 * @LastEditTime : 2024-08-18 16:15:29
 * @Copyright    : ZFCompany AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2024.
 * @Email: 15056030055@163.com
**/

#ifndef _FUNC_H_
#define _FUNC_H_
#include <myhead.h>
#include <sqlite3.h>
#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <ctype.h>
#include "weather.h"


// 清理字符串中的非打印字符
void clean_string(char *str);

/**
 * @brief        : 创建或打开数据库
 * @param         {char*} filename:数据库名
 * @param         {char*} table:表格名
 * @param         {char*} __fmt:表格内部成员
 * @return        {*}
**/
sqlite3* OpenDataBase(const char* filename, const char* table, const char* __fmt, ...);

/**
 * @brief        : 字符串拼接
 * @param         {char*} buf:要加入的字符串
 * @return        {*}返回拼接后的字符串
**/
char* mysprintf(char* buf);


/**
 * @brief        : 根据name和passwd搜索数据库时候有一样的
 * @param         {char const*} name:
 * @param         {char const*} passwd:
 * @return        {*}查找成功返回1,未查询到返回0
**/
int callback(void* arg, int cols, char **col_text, char ** col_name);

/**
 * @brief        : 数据库查找函数
 * @param         {sqlite3} *ppDb:
 * @param         {client_t} usr:用户数据的指针
 * @param         {char*} tabname:表格名
 * @return        {*}
**/
int do_search(sqlite3 *ppDb, client_t usr, const char* tabname);

/**
 * @brief        : 将用户消息放入数据库
 * @param         {sqlite3} *ppDb:
 * @param         {client_t} usr:用户数据的指针
 * @param         {char*} tabname:表格名
 * @return        {*} 成功返回0， 失败返回-1
**/
int insert_info(sqlite3 *ppDb, client_t usr, const char* tabname);

/**
 * @brief        :判断注册还是登录
 * @param         {sqlite3} *ppDb:
 * @param         {client_t} usr:
 * @return        {*}成功返回0， 失败返回-1
**/
int RegistOrSign(sqlite3 *ppDb, client_t usr, const char* tabname);

/**********************************GET URL************************************** */
/**********************************GET URL************************************** */
/**
 * @brief        : 回调函数，用于处理libcurl获取的数据
 * @param         {void} *contents:指向 libcurl 从服务器接收到的数据的指针
 * @param         {size_t} size:每个数据块的大小
 * @param         {size_t} nmemb:数据块的数量
 * @param         {void} *userp:用户数据指针，在调用 curl_easy_setopt 时设置，它指向将要保存数据的变量
 * @return        {*}
**/
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);

/**
 * @brief        : 查看json数据
 * @param         {char} *json_data:
 * @return        {*}
**/
void parse_weather_data(const char *json_data);

/**
 * @brief        : 
 * @param         {CURL*} curl:
 * @param         {char*} pos:
 * @return        {*}
**/
char* get_URL(CURL* curl, const char* pos);

/**
 * @brief        : 将weather_t 结构体转换为 JSON 对象
 * @param         {weather_t*} weath:
 * @return        {*}
**/
cJSON* weather_to_json(const weather_t* weath);

/**
 * @brief        :将 client_t 结构体转换为 JSON 对象
 * @param         {client_t*} client:
 * @return        {*}
**/
cJSON* client_to_json(const client_t* client);

/**
 * @brief        : 将 total_t 结构体转换为 JSON 对象
 * @param         {total_t*} total:
 * @return        {*}
**/
char* total_to_json(const total_t* total);

int json_to_weather(const cJSON *json, weather_t *weath);

int json_to_client(const cJSON *json, client_t *client);

int json_to_total(const char *json_string, total_t *total);

//将URL的数据放入weather_t中
int parse_weather_json(const char *json_string, weather_t *weath);
#endif // !_FUNC_H_