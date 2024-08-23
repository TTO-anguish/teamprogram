/**
 * @FilePath     : /home/ZF/bottom_program/server_item/json.c
 * @Description  :  
 * @Author       : CMH,ZF,ZY,SSS
 * @Version      : 0.0.1
 * @LastEditors  : zongfei
 * @LastEditTime : 2024-08-23 10:15:17
**/
#include "json.h"

cJSON* envir_to_json(envir_t* envir) {
    cJSON *json_envir = cJSON_CreateObject();
    cJSON_AddNumberToObject(json_envir, "hum", envir->hum);
    cJSON_AddNumberToObject(json_envir, "tem", envir->tem);
    cJSON_AddNumberToObject(json_envir, "sunshine", envir->sunshine);
    cJSON_AddNumberToObject(json_envir, "NH3", envir->NH3);
    return json_envir;
}

cJSON* warn_to_json(warn_t* warn) {
    cJSON *json_warn = cJSON_CreateObject();
    cJSON_AddNumberToObject(json_warn, "flag_NH3", warn->flag_NH3);
    cJSON_AddNumberToObject(json_warn, "flag_tem", warn->flag_tem);
    return json_warn;
}

char* corrToQt_to_json(corrToQt_t* data) {
    cJSON *root = cJSON_CreateObject();

    // 添加环境数据
    cJSON *json_envir = envir_to_json(&data->enviro);
    cJSON_AddItemToObject(root, "enviro", json_envir);

    // 添加警告数据
    cJSON *json_warn = warn_to_json(&data->warn);
    cJSON_AddItemToObject(root, "warn", json_warn);

    // 添加时间和风扇状态
    cJSON_AddNumberToObject(root, "time", (double)data->time);
    cJSON_AddNumberToObject(root, "fans", data->fans);
    cJSON_AddNumberToObject(root, "sqlsearch", data->sqlsearch);

    // 将 JSON 转为字符串
    char *json_string = cJSON_Print(root);
    
    // 释放 JSON 对象
    cJSON_Delete(root);

    return json_string;
}

void json_to_envir(cJSON* json_envir, envir_t* envir) {
    envir->hum = cJSON_GetObjectItem(json_envir, "hum")->valuedouble;  //没有float类型，故精度会损失
    envir->tem = cJSON_GetObjectItem(json_envir, "tem")->valuedouble;
    envir->sunshine = cJSON_GetObjectItem(json_envir, "sunshine")->valuedouble;
    envir->NH3 = cJSON_GetObjectItem(json_envir, "NH3")->valuedouble;
}

void json_to_warn(cJSON* json_warn, warn_t* warn) {
    warn->flag_NH3 = cJSON_GetObjectItem(json_warn, "flag_NH3")->valueint;
    warn->flag_tem = cJSON_GetObjectItem(json_warn, "flag_tem")->valueint;
}

void json_to_corrToQt(const char* json_string, corrToQt_t* data) {
    cJSON *root = cJSON_Parse(json_string);
    if (root == NULL) {
        printf("Invalid JSON\n");
        return;
    }

    // 解析环境数据
    cJSON *json_envir = cJSON_GetObjectItem(root, "enviro");
    if (json_envir != NULL) {
        json_to_envir(json_envir, &data->enviro);
    }

    // 解析警告数据
    cJSON *json_warn = cJSON_GetObjectItem(root, "warn");
    if (json_warn != NULL) {
        json_to_warn(json_warn, &data->warn);
    }

    // 解析时间和风扇状态
    data->time = (time_t)cJSON_GetObjectItem(root, "time")->valuedouble;
    data->fans = cJSON_GetObjectItem(root, "fans")->valueint;
    data->sqlsearch = cJSON_GetObjectItem(root, "sqlsearch")->valueint;

    // 释放 JSON 对象
    cJSON_Delete(root);
}