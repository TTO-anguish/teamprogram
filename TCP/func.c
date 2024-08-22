/**
 * @FilePath     : /home/ZF/TCP/func.c
 * @Description  : 各种封装的函数
 * @Author       : zongfei
 * @Version      : 0.0.1
 * @LastEditors  : zongfei
 * @LastEditTime : 2024-08-18 16:19:09
 * @Copyright    : ZFCompany AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2024.
 * @Email: 15056030055@163.com
**/

#include "func.h"   

// 清理字符串中的非打印字符
void clean_string(char *str) {
    char *src = str;
    char *dst = str;

    while (*src) {
        if (isprint((unsigned char)*src)) {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
}

sqlite3* OpenDataBase(const char* filename, const char* table, const char* __fmt, ...)
{
    sqlite3* ppDb;
    if(sqlite3_open(filename, &ppDb) != SQLITE_OK){
        printf("open error:%d, %s\n", sqlite3_errcode(ppDb), sqlite3_errmsg(ppDb));
        return NULL;
    }
    printf("数据库已打开\n");
    //创建用户表格，保存用户的名称和密码,表格命：userinfo
    char *errmsg = NULL;
    char sql[512] = "";
    memset(sql, 0, sizeof(sql));
    snprintf(sql, sizeof(sql), "create table if not exists %s (%s);" ,table, __fmt);

    if(sqlite3_exec(ppDb, sql, NULL, NULL, &errmsg) != SQLITE_OK){
        printf("%s create error: %s\n", table, errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(ppDb);
        return NULL;
    }
    printf("%s table create success!\n", table);

    return ppDb;
}

int PersonFlag = 0;//判断是否查到人员(0:查找失败 1:已找到)

int callback(void* arg, int cols, char **col_text, char ** col_name) 
{
    int flag_password = 0, flag_Name = 0;
    client_t temp = *(client_t*)(arg);

    if(PersonFlag == 0){
        for(int i = 0;i < cols; i++){
            //if(temp.password == col_text[i]) flag_password = 1;
            if(strcmp(temp.password, col_text[i]) == 0) flag_password = 1;
            printf("%s\n", col_text[i]);
            //if(temp.user_id == col_text[i]) flag_ID = 1;
            if(strcmp(temp.name, col_text[i]) == 0) flag_Name = 1;

            if((flag_password && flag_Name) == 1) break; //找到了就提前退出
        }
        PersonFlag = (flag_password && flag_Name);
    } 
    
    //flag_ID = flag_password = 0;  //重新赋值为0,防止出现不同行不同列出现相同的 要找的 值
                                   //不能加，加了有错误,验证前面的会一直失败
    return 0;
}

int do_search(sqlite3 *ppDb, client_t usr, const char* tabname)
{
    char sql[64] = "";
    memset(sql, 0, sizeof(sql));
    snprintf(sql, sizeof(sql), "select * from %s;", tabname);
    char* err_message = NULL;

    //传入用户登录信息的结构体，判断密码等是否正确
    if(sqlite3_exec(ppDb, sql, callback, &usr, &err_message) != SQLITE_OK) {
        printf("userinfo search error:%s\n", err_message);
        sqlite3_free(err_message);
        return -1;
    }

    printf("查找结束!\n");
    return PersonFlag;  
    //本来想着通过回调函数的返回值处理，但是不行，因此用了全局或者静态
    //或者使用SQLite的预处理语句和绑定使用SQLite的预处理语句（sqlite3_prepare_v2）
    //和绑定（sqlite3_bind函数系列）。这样，你可以控制查询的执行，并逐行处理结果
}

int insert_info(sqlite3 *ppDb, client_t usr, const char* tabname)
{
    char sql[128] = "";
    memset(sql, 0, sizeof(sql));
    snprintf(sql, sizeof(sql), "insert into %s values(?, ?);", tabname);

    //准备 SQL 语句        
    sqlite3_stmt* stmt;
    if(sqlite3_prepare_v2(ppDb, sql, -1, &stmt, NULL) != SQLITE_OK){
        printf("Failed to prepare statement: %s\n", sqlite3_errmsg(ppDb));
        return -1;
    }

    // 绑定数据到 SQL 语句的参数(不在sql语句中填写，在这里注入，避免字符潜在问题和sql注入攻击)
    //SQLITE_STATIC：绑定参数时不需要管理绑定数据的内存
    sqlite3_bind_text(stmt, 1, usr.name, -1, SQLITE_STATIC);   //第一个参数绑定 name
    sqlite3_bind_text(stmt, 2, usr.password, -1, SQLITE_STATIC);// 第二个参数绑定 passwd

    printf("usr.name = %s\n", usr.name);
    printf("usr.password = %s\n", usr.password);

    //执行sql语句
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        printf("Execution failed: %s\n", sqlite3_errmsg(ppDb));
        sqlite3_finalize(stmt);
        return -1;
    }
    
    sqlite3_finalize(stmt);//释放资源
    printf("数据插入成功！！！！！！！！！！！\n");
    return 0;   //成功返回0
}

int RegistOrSign(sqlite3 *ppDb, client_t usr, const char* tablename)
{
    client_t client = usr;
    if(usr.flag){
        if(do_search(ppDb, client, tablename) == 1){  //存在
            return 0;
        }
        else{
            printf("未找到对应客户\n");
            return -1;
        }
    }
    else{  //注册人员
        //信息入数据库
        insert_info(ppDb, client, tablename);
    }
    return 0;
}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;  //计算数据实际大小
    char **response_ptr = (char**)userp; //获取指向响应数据缓冲区的指针

    // 使用realloc扩展缓冲区大小，即重新分配内存
    *response_ptr = realloc(*response_ptr, strlen(*response_ptr) + realsize + 1);
    if (*response_ptr == NULL) {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    // 将新数据追加到缓冲区末尾
    strncat(*response_ptr, contents, realsize);

    return realsize;
}

void parse_weather_data(const char *json_data) {
    printf("Received JSON data: %s\n", json_data);  // 调试输出完整的JSON数据

    //查看json的具体数据
    cJSON *root = cJSON_Parse(json_data);
    if (root == NULL) {
        printf("Error before: [%s]\n", cJSON_GetErrorPtr());
        return;
    }

    cJSON *results = cJSON_GetObjectItem(root, "results");
    if (results == NULL || !cJSON_IsArray(results)) {
        printf("Error: 'results' field is missing or is not an array\n");
        cJSON_Delete(root);
        return;
    }

    cJSON *first_result = cJSON_GetArrayItem(results, 0);
    if (first_result == NULL) {
        printf("Error: 'results' array is empty\n");
        cJSON_Delete(root);
        return;
    }

    cJSON *location = cJSON_GetObjectItem(first_result, "location");
    cJSON *now = cJSON_GetObjectItem(first_result, "now");

    if (location != NULL) {
        cJSON *name = cJSON_GetObjectItem(location, "name");
        cJSON *path = cJSON_GetObjectItem(location, "path");
        cJSON *timezone = cJSON_GetObjectItem(location, "timezone");
        if (name && path && timezone) {
            printf("Location: %s, %s, %s\n", name->valuestring, path->valuestring, timezone->valuestring);
        } 
        else {
            printf("Error: Missing location details\n");
        }
    } 
    else {
        printf("Error: 'location' field is missing\n");
    }

    if (now != NULL) {
        cJSON *text = cJSON_GetObjectItem(now, "text");
        cJSON *temperature = cJSON_GetObjectItem(now, "temperature");
        cJSON *pressure = cJSON_GetObjectItem(now, "pressure");
        cJSON *humidity = cJSON_GetObjectItem(now, "humidity");
        cJSON *visibility = cJSON_GetObjectItem(now, "visibility");
        cJSON *wind_direction = cJSON_GetObjectItem(now, "wind_direction");
        cJSON *wind_speed = cJSON_GetObjectItem(now, "wind_speed");
        cJSON *wind_scale = cJSON_GetObjectItem(now, "wind_scale");

        if (text && temperature && pressure && humidity && visibility && \
            wind_direction && wind_speed && wind_scale) {
            printf("Weather: %s\n", text->valuestring);
            printf("Temperature: %s°C\n", temperature->valuestring);
            printf("Pressure: %s mb\n", pressure->valuestring);
            printf("Humidity: %s%%\n", humidity->valuestring);
            printf("Visibility: %s km\n", visibility->valuestring);
            printf("Wind: %s at %s km/h (Scale: %s)\n", 
                   wind_direction->valuestring, wind_speed->valuestring, wind_scale->valuestring);
        } else {
            printf("Error: Missing current weather details\n");
        }
    } else {
        printf("Error: 'now' field is missing\n");
    }
    
    printf("sssssssssssssssssssssssssssss\n");
    
    cJSON_Delete(root);
}

char* get_URL(CURL* curl, const char* pos)
{
    char *response = calloc(1, sizeof(char));
    char url[132] = "";
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    sprintf(url, "https://api.seniverse.com/v3/weather/now.json?key=SshN2H8uCzWIromB9&location=%s&language=zh-Hans&unit=c", pos); 
    if(curl){
        curl_easy_setopt(curl, CURLOPT_URL, url);  //请求URL
    }
    //将数据写入char*结构
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    //执行请求
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }
    else parse_weather_data(response); //解析并打印数据
    return response;
}


cJSON* weather_to_json(const weather_t* weath)
{
    cJSON *json = cJSON_CreateObject();
    if (json == NULL) {
        return NULL;
    }

    cJSON_AddStringToObject(json, "temperature", weath->temperature);
    cJSON_AddStringToObject(json, "name", weath->name);
    cJSON_AddStringToObject(json, "country", weath->country);
    cJSON_AddStringToObject(json, "weather", weath->weather);
    cJSON_AddStringToObject(json, "pressure", weath->pressure);
    cJSON_AddStringToObject(json, "humidity", weath->humidity);
    cJSON_AddStringToObject(json, "visibility", weath->visibility);
    cJSON_AddStringToObject(json, "wind_direction", weath->wind_direction);
    cJSON_AddStringToObject(json, "wind_speed", weath->wind_speed);
    cJSON_AddStringToObject(json, "wind_scale", weath->wind_scale);

    return json;
}


cJSON* client_to_json(const client_t* client)
{
    cJSON *json = cJSON_CreateObject();
    if (json == NULL) {
        return NULL;
    }

    cJSON_AddStringToObject(json, "name", client->name);
    cJSON_AddStringToObject(json, "password", client->password);
    cJSON_AddNumberToObject(json, "flag", client->flag);
    cJSON_AddStringToObject(json, "position", client->position);
    cJSON_AddNumberToObject(json, "exist", client->exist);
    cJSON_AddNumberToObject(json, "newfd", client->newfd);

    return json;
}


char* total_to_json(const total_t* total)
{
    cJSON *json = cJSON_CreateObject();
    if (json == NULL) {
        return NULL;
    }

    // 添加 weather_t 部分
    cJSON *weath_json = weather_to_json(&total->weath);
    if (weath_json == NULL) {
        cJSON_Delete(json);
        return NULL;
    }
    cJSON_AddItemToObject(json, "weath", weath_json);

    // 添加 client_t 部分
    cJSON *client_json = client_to_json(&total->total_cli);
    if (client_json == NULL) {
        cJSON_Delete(json);
        return NULL;
    }
    cJSON_AddItemToObject(json, "total_cli", client_json);

    // 添加其他字段
    // cJSON_AddNumberToObject(json, "exist", total->exist);
    // cJSON_AddNumberToObject(json, "newfd", total->newfd);

    // 将 JSON 对象转换为字符串
    char *json_string = cJSON_Print(json);
    cJSON_Delete(json); // 释放 JSON 对象内存

    return json_string;
}


int json_to_weather(const cJSON *json, weather_t *weath)
{
    cJSON *temp = cJSON_GetObjectItemCaseSensitive(json, "temperature");
    cJSON *name = cJSON_GetObjectItemCaseSensitive(json, "name");
    cJSON *country = cJSON_GetObjectItemCaseSensitive(json, "country");
    cJSON *weather = cJSON_GetObjectItemCaseSensitive(json, "weather");
    cJSON *pressure = cJSON_GetObjectItemCaseSensitive(json, "pressure");
    cJSON *humidity = cJSON_GetObjectItemCaseSensitive(json, "humidity");
    cJSON *visibility = cJSON_GetObjectItemCaseSensitive(json, "visibility");
    cJSON *wind_direction = cJSON_GetObjectItemCaseSensitive(json, "wind_direction");
    cJSON *wind_speed = cJSON_GetObjectItemCaseSensitive(json, "wind_speed");
    cJSON *wind_scale = cJSON_GetObjectItemCaseSensitive(json, "wind_scale");

    if (cJSON_IsString(temp) && cJSON_IsString(name) && cJSON_IsString(country) &&
        cJSON_IsString(weather) && cJSON_IsString(pressure) && cJSON_IsString(humidity) &&
        cJSON_IsString(visibility) && cJSON_IsString(wind_direction) &&
        cJSON_IsString(wind_speed) && cJSON_IsString(wind_scale)) {
        strncpy(weath->temperature, temp->valuestring, sizeof(weath->temperature) - 1);
        strncpy(weath->name, name->valuestring, sizeof(weath->name) - 1);
        strncpy(weath->country, country->valuestring, sizeof(weath->country) - 1);
        strncpy(weath->weather, weather->valuestring, sizeof(weath->weather) - 1);
        strncpy(weath->pressure, pressure->valuestring, sizeof(weath->pressure) - 1);
        strncpy(weath->humidity, humidity->valuestring, sizeof(weath->humidity) - 1);
        strncpy(weath->visibility, visibility->valuestring, sizeof(weath->visibility) - 1);
        strncpy(weath->wind_direction, wind_direction->valuestring, sizeof(weath->wind_direction) - 1);
        strncpy(weath->wind_speed, wind_speed->valuestring, sizeof(weath->wind_speed) - 1);
        strncpy(weath->wind_scale, wind_scale->valuestring, sizeof(weath->wind_scale) - 1);
        return 0; // 成功
    }
    return -1; // 失败
}


int json_to_client(const cJSON *json, client_t *client) {
    cJSON *name = cJSON_GetObjectItemCaseSensitive(json, "name");
    cJSON *password = cJSON_GetObjectItemCaseSensitive(json, "password");
    cJSON *flag = cJSON_GetObjectItemCaseSensitive(json, "flag");
    cJSON *position = cJSON_GetObjectItemCaseSensitive(json, "position");
    cJSON *exist = cJSON_GetObjectItemCaseSensitive(json, "exist");
    cJSON *newfd = cJSON_GetObjectItemCaseSensitive(json, "newfd");

    if (cJSON_IsString(name) && cJSON_IsString(password) && cJSON_IsNumber(flag) &&
        cJSON_IsString(position) && cJSON_IsNumber(exist) && cJSON_IsNumber(newfd)) {
        strncpy(client->name, name->valuestring, sizeof(client->name) - 1);
        strncpy(client->password, password->valuestring, sizeof(client->password) - 1);
        client->flag = flag->valueint;
        strncpy(client->position, position->valuestring, sizeof(client->position) - 1);
        client->exist = exist->valueint;
        client->newfd = newfd->valueint;

        return 0; // 成功
    }
    return -1; // 失败
}


int json_to_total(const char *json_string, total_t *total) {
    cJSON *json = cJSON_Parse(json_string);
    if (json == NULL) {
        return -1; // 解析失败
    }

    cJSON *weather_json = cJSON_GetObjectItemCaseSensitive(json, "weath");
    cJSON *client_json = cJSON_GetObjectItemCaseSensitive(json, "total_cli");
    // cJSON *exist = cJSON_GetObjectItemCaseSensitive(json, "exist");
    // cJSON *newfd = cJSON_GetObjectItemCaseSensitive(json, "newfd");

    if (weather_json == NULL || client_json == NULL) {
        cJSON_Delete(json);
        return -1; // 数据不完整
    }

    if (json_to_weather(weather_json, &total->weath) != 0 ||
        json_to_client(client_json, &total->total_cli) != 0) {
        cJSON_Delete(json);
        return -1; // 解析失败
    }

    // total->exist = exist->valueint;
    // total->newfd = newfd->valueint;

    cJSON_Delete(json); // 释放 JSON 对象内存
    return 0; // 成功
}


int parse_weather_json(const char *json_string, weather_t *weath) {
    // 解析 JSON 字符串
    cJSON *json = cJSON_Parse(json_string);
    if (json == NULL) {
        return -1; // 解析失败
    }

    // 提取 "location" 和 "now" 对象
    cJSON *results = cJSON_GetObjectItemCaseSensitive(json, "results");
    if (!cJSON_IsArray(results) || cJSON_GetArraySize(results) == 0) {
        cJSON_Delete(json);
        return -1; // "results" 不是数组或数组为空
    }

    cJSON *location_obj = cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(results, 0), "location");
    cJSON *now_obj = cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(results, 0), "now");

    if (location_obj == NULL || now_obj == NULL) {
        cJSON_Delete(json);
        return -1; // "location" 或 "now" 对象不存在
    }

    // 解析 location 对象中的 name 和 country
    cJSON *name = cJSON_GetObjectItemCaseSensitive(location_obj, "name");
    cJSON *country = cJSON_GetObjectItemCaseSensitive(location_obj, "country");

    if (cJSON_IsString(name) && cJSON_IsString(country)) {
        strncpy(weath->name, name->valuestring, sizeof(weath->name) - 1);
        strncpy(weath->country, country->valuestring, sizeof(weath->country) - 1);
    } else {
        cJSON_Delete(json);
        return -1; // 无法解析 name 或 country
    }

    // 解析 now 对象中的其他字段
    cJSON *temperature = cJSON_GetObjectItemCaseSensitive(now_obj, "temperature");
    cJSON *weather = cJSON_GetObjectItemCaseSensitive(now_obj, "code");
    cJSON *pressure = cJSON_GetObjectItemCaseSensitive(now_obj, "pressure");
    cJSON *humidity = cJSON_GetObjectItemCaseSensitive(now_obj, "humidity");
    cJSON *visibility = cJSON_GetObjectItemCaseSensitive(now_obj, "visibility");
    cJSON *wind_direction = cJSON_GetObjectItemCaseSensitive(now_obj, "wind_direction");
    cJSON *wind_speed = cJSON_GetObjectItemCaseSensitive(now_obj, "wind_speed");
    cJSON *wind_scale = cJSON_GetObjectItemCaseSensitive(now_obj, "wind_scale");

    if (cJSON_IsString(temperature) && cJSON_IsString(weather) &&
        cJSON_IsString(pressure) && cJSON_IsString(humidity) &&
        cJSON_IsString(visibility) && cJSON_IsString(wind_direction) &&
        cJSON_IsString(wind_speed) && cJSON_IsString(wind_scale)) {

        strncpy(weath->temperature, temperature->valuestring, sizeof(weath->temperature) - 1);
        weath->temperature[sizeof(weath->temperature) - 1] = '\0'; // 确保字符串终结
        clean_string(weath->temperature);

        strncpy(weath->weather, weather->valuestring, sizeof(weath->weather) - 1);
        weath->weather[sizeof(weath->weather) - 1] = '\0'; // 确保字符串终结
        clean_string(weath->weather);

        strncpy(weath->pressure, pressure->valuestring, sizeof(weath->pressure) - 1);
        weath->pressure[sizeof(weath->pressure) - 1] = '\0'; // 确保字符串终结
        clean_string(weath->pressure);
        
        strncpy(weath->humidity, humidity->valuestring, sizeof(weath->humidity) - 1);
        weath->humidity[sizeof(weath->humidity) - 1] = '\0'; // 确保字符串终结
        clean_string(weath->humidity);

        strncpy(weath->visibility, visibility->valuestring, sizeof(weath->visibility) - 1);
        weath->visibility[sizeof(weath->visibility) - 1] = '\0'; // 确保字符串终结
        clean_string(weath->visibility);

        strncpy(weath->wind_direction, wind_direction->valuestring, sizeof(weath->wind_direction) - 1);
        weath->wind_direction[sizeof(weath->wind_direction) - 1] = '\0'; // 确保字符串终结
        clean_string(weath->wind_direction);

        strncpy(weath->wind_speed, wind_speed->valuestring, sizeof(weath->wind_speed) - 1);
        weath->wind_speed[sizeof(weath->wind_speed) - 1] = '\0'; // 确保字符串终结
        clean_string(weath->wind_speed);

        strncpy(weath->wind_scale, wind_scale->valuestring, sizeof(weath->wind_scale) - 1);
        weath->wind_scale[sizeof(weath->wind_scale) - 1] = '\0'; // 确保字符串终结
        clean_string(weath->wind_scale);
    } else {
        cJSON_Delete(json);
        return -1; // 无法解析某些 now 对象中的字段
    }

    cJSON_Delete(json); // 释放 JSON 对象内存
    return 0; // 成功
}