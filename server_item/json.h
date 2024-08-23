/**
 * @FilePath     : /home/ZF/bottom_program/server_item/json.h
 * @Description  :  
 * @Author       : CMH,ZF,ZY,SSS
 * @Version      : 0.0.1
 * @LastEditors  : zongfei
 * @LastEditTime : 2024-08-23 11:19:29
**/
#ifndef _JSON_H_
#define _JSON_H_

#include <myhead.h>
#include "cJSON.h"
#include "transducer_struct.h"

/**************************结构体转JSON格式字符串 ******************************/
//envir_t 转 JSON
cJSON* envir_to_json(envir_t* envir);

//warn_t 转 JSON
cJSON* warn_to_json(warn_t* warn);

//total_t 转 JSON格式的字符串
char* corrToQt_to_json(corrToQt_t* data);


/***************************json 转 total_t结构体 ****************************/

//json 转 ervir_t
void json_to_envir(cJSON* json_envir, envir_t* envir);

//json 转 warn_t
void json_to_warn(cJSON* json_warn, warn_t* warn);

//json 转 corrToQt_t
void json_to_corrToQt(const char* json_string, corrToQt_t* data);

#endif // !_JSON_H_