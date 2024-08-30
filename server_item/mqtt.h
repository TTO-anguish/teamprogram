/**
 * @FilePath     : /home/ZF/bottom_program/server_item/mqtt.h
 * @Description  :  
 * @Author       : CMH,ZF,ZY,SSS
 * @Version      : 0.0.1
 * @LastEditors  : zongfei
 * @LastEditTime : 2024-08-23 11:44:14
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
#include "cJSON.h"
#include "sqlite.h"
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

//客户端退出函数
void exit_mqtt(MQTTClient* client);

// 自定义订阅函数
/*handle：这是一个 MQTTClient 类型的句柄，代表客户端的连接实例。
    你在调用 MQTTClient_create 或 MQTTClient_createWithOptions 时获得这个句柄。
    topic：一个 C 字符串，表示你要订阅的 MQTT 主题。主题必须遵循 MQTT 主题命名规则。
    qos：指定订阅的服务质量（QoS）。有效的 QoS 值为 0、1 或 2：
    QoS 0: 至多一次传输。消息可能会丢失或重复。
    QoS 1: 至少一次传输。消息不会丢失，但可能会重复。
    QoS 2: 只有一次传输。消息不会丢失或重复。*/
int mqtt_subscribe(MQTTClient* client, const char *topic);

//自定义发布函数
int mqtt_publish(const char *topic, char *msg, MQTTClient* client);

#endif // !__MQTT_H_