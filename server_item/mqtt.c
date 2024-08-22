/**
 * @FilePath     : /home/ZF/bottom_program/server_item/mqtt.c
 * @Description  :  
 * @Author       : CMH,ZF,ZY,SSS
 * @Version      : 0.0.1
 * @LastEditors  : zongfei
 * @LastEditTime : 2024-08-22 22:06:37
**/
#include "mqtt.h"

/*MQTTClient_deliveryToken结构体用于表示消息的发送状态。它通常用于在 QoS 1 
或 QoS 2 的消息传递中确认消息是否已经成功发送到 MQTT 服务器*/
volatile MQTTClient_deliveryToken deliveredtoken;   //SB,就他妈是个int类型
char data[100];

char *l_trim(char *szOutput, const char *szInput)
{
    assert(szInput != NULL);
    assert(szOutput != NULL);
    assert(szOutput != szInput);
    for (NULL; *szInput != '\0' && isspace(*szInput); ++szInput)
    {
        ;
    }
    return strcpy(szOutput, szInput);
}

/*   删除右边的空格   */
char *r_trim(char *szOutput, const char *szInput)
{
    char *p = NULL;
    assert(szInput != NULL);
    assert(szOutput != NULL);
    assert(szOutput != szInput);
    strcpy(szOutput, szInput);
    for (p = szOutput + strlen(szOutput) - 1; p >= szOutput && isspace(*p); --p)
    {
        ;
    }
    *(++p) = '\0';
    return szOutput;
}

/*   删除两边的空格   */
char *a_trim(char *szOutput, const char *szInput)
{
    char *p = NULL;
    assert(szInput != NULL);
    assert(szOutput != NULL);
    l_trim(szOutput, szInput);
    for (p = szOutput + strlen(szOutput) - 1; p >= szOutput && isspace(*p); --p);
    *(++p) = '\0';
    return szOutput;
}

int GetProfileString(char *profile, char *AppName, char *KeyName, char *KeyVal)
{
    char appname[32], keyname[32];
    char *buf, *c;
    char buf_i[KEYVALLEN], buf_o[KEYVALLEN];
    FILE *fp;
    int found = 0; /* 1 AppName 2 KeyName */
    if ((fp = fopen(profile, "r")) == NULL)
    {
        printf("openfile [%s] error [%s]\n", profile, strerror(errno));
        return (-1);
    }
    fseek(fp, 0, SEEK_SET);
    memset(appname, 0, sizeof(appname));
    sprintf(appname, "[%s]", AppName);

    while (!feof(fp) && fgets(buf_i, KEYVALLEN, fp) != NULL)
    {
        l_trim(buf_o, buf_i);
        if (strlen(buf_o) <= 0)
            continue;
        buf = NULL;
        buf = buf_o;

        if (found == 0){
            if (buf[0] != '[') continue;
            else if (strncmp(buf, appname, strlen(appname)) == 0){
                found = 1;
                continue;
            }
        }
        else if (found == 1){
            if (buf[0] == '#') continue;
            else if (buf[0] == '[') break;
            else{
                if ((c = (char *)strchr(buf, '=')) == NULL) continue;        
                memset(keyname, 0, sizeof(keyname));

                sscanf(buf, "%[^=|^ |^\t]", keyname);
                if (strcmp(keyname, KeyName) == 0)
                {
                    sscanf(++c, "%[^\n]", KeyVal);
                    char *KeyVal_o = (char *)malloc(strlen(KeyVal) + 1);
                    if (KeyVal_o != NULL)
                    {
                        memset(KeyVal_o, 0, sizeof(KeyVal_o));
                        a_trim(KeyVal_o, KeyVal);
                        if (KeyVal_o && strlen(KeyVal_o) > 0)
                            strcpy(KeyVal, KeyVal_o);
                        free(KeyVal_o);
                        KeyVal_o = NULL;
                    }
                    found = 2;
                    break;
                }
                else continue;
            }
        }
    }
    fclose(fp);
    if (found == 2) return (0);   
    else return (-1);    
}

void delivered(void *context, MQTTClient_deliveryToken dt)
{
	// printf("Message with token value %d delivery confirmed\n", dt);
	deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
	// 数据到达后执行这个函数并且把数据存放到数组里
	memset(data, 0, sizeof(data));
	memcpy(data, message->payload, message->payloadlen);
	// 打印出订阅到的数据
	// printf("data_sub:%s\n",data);
	//transfor_virtual_data(); //要改成自己的
	MQTTClient_freeMessage(&message);
	MQTTClient_free(topicName);
	return 1;
}

// 连接丢失时执行
void connlost(void *context, char *cause)
{
	printf("\nConnection lost\n");
	printf(" cause: %s\n", cause);
}

void MQTTconnect(MQTTClient* client, char* client_id)
{
    char uri[128] = {0};
    int rc = 0;
    //将所有字段设置为默认值，确保不会遗漏初始化
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

    GetProfileString(DEF_CONF_FILE, "mqtt", "uri", uri);//得到mqtt的ip地址和端口号
    // 创建 MQTT 客户端实例
    MQTTClient_create(client, uri, client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;  // 设置保持连接的时间间隔
    conn_opts.cleansession = 1;  // 设置是否清除会话（0 为不清除，1 为清除）

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    printf("mqtt connect success\n");
}