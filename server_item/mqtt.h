/**
 * @FilePath     : /home/ZF/bottom_program/server_item/mqtt.h
 * @Description  :  
 * @Author       : CMH,ZF,ZY,SSS
 * @Version      : 0.0.1
 * @LastEditors  : zongfei
 * @LastEditTime : 2024-08-22 22:02:31
**/
#ifndef __MQTT_H_
#define __MQTT_H_
#define DEF_CONF_FILE   "./smartHome.conf"
#define KEYVALLEN 100

#include <MQTTClient.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "json.h"
#include "transducer_struct.h"

void delivered(void *context, MQTTClient_deliveryToken dt);


//消息到达后
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message);

//连接丢失时
void connlost(void *context, char *cause);

//删除右边空格
char *r_trim(char *szOutput, const char *szInput);

//删除左边空格
char *l_trim(char *szOutput, const char *szInput);

/*   删除两边的空格   */
char *a_trim(char *szOutput, const char *szInput);

/**
 * @brief        : 将名为profile文件的2个键名的内容放到KeyVal字符串中
 * @param         {char} *profile:文件名
 * @param         {char} *AppName:键名
 * @param         {char} *KeyName:键名
 * @param         {char} *KeyVal: 
 * @return        {*}
**/
int GetProfileString(char *profile, char *AppName, char *KeyName, char *KeyVal);

/**
 * @brief        : MQTT初始化并连接
 * @param         {MQTTClient*} client:
 * @param         {char*} client_id:
 * @return        {*}
**/
void MQTTconnect(MQTTClient* client, char* client_id);


#endif // !__MQTT_H_