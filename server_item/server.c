/**
 * @FilePath     : /home/ZF/bottom_program/server_item/server.c
 * @Description  :
 * @Author       : zongfei
 * @Version      : 0.0.1
 * @LastEditors  : zongfei
 * @LastEditTime : 2024-08-30 19:46:30
 * @Copyright    : ZFCompany AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2024.
 * @Email: 15056030055@163.com
 **/
#include <myhead.h>
#include "cJSON.h"
#include <MQTTClient.h>
#include <sqlite3.h>
#include "sqlite.h"
#include "mqtt.h"
#include "transducer_struct.h"
#define ServerIp "172.19.113.181"
#define Port 8888
#define sqlname "Client.db"
#define SUB "SSSToZF"
#define PUB "ZFToSSS"

MQTTClient client;
extern char data[150];
extern int recvMQTTFlag;
int msgid;

void *PUB_func(void *buf)
{
    printf("进入  发送给QT的线程  成功\n");
    char databuf[150] = {0};
    corrToQt_t corr = {
        .enviro = {
            .hum = 22,
            .NH3 = 13.222,
            .sunshine = 88.1324,
            .tem = 29},
        .fans = 2,
        .warn = {.flag_NH3 = 1, .flag_tem = 1},
        .sqlsearch = 2
    };

    while(1){
        //databuf = corrToQt_to_json(&corr);
        strcpy(databuf, corrToQt_to_json(&corr));
        mqtt_publish(PUB ,databuf, &client);
    }
}

void *SUB_func(void *buf)
{
    printf("进入  接受QT消息的线程  成功\n");
    corrToQt_t corr;
    if (MQTTClient_isConnected(client)) printf("Client is connected.\n");   
    else {
        fprintf(stderr, "Client is not connected.\n");
        return NULL;
    }

    //订阅消息
    int rc = mqtt_subscribe(&client, "SSSToZF");
    if (rc != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "Failed to subscribe, return code %d\n", rc);
        return NULL;
        // 根据返回值处理错误
    } else printf("Subscribed to topic successfully.\n");

    while (1){

        if(recvMQTTFlag == 1){
            printf("data = %s", data);
            json_to_corrToQt(data, &corr);

            printf("corrToQt_t->enviro = %f, %f, %f, %f\n", corr.enviro.hum,
               corr.enviro.NH3, corr.enviro.sunshine, corr.enviro.tem);
            
            recvMQTTFlag = 0;//将标志位重置为0
            memset(data, 0, sizeof(data)); 
        }
        usleep(100000);
    }
}

int main(int argc, const char *argv[])
{
    pthread_t tid1, tid2;

    key_t key = ftok("./ipc", 1);
    if (key == -1)
    {
        perror("ftok err");
        return -1;
    }
    if ((msgid = msgget(key, IPC_CREAT | 0666)) == -1)
    {
        perror("msgget error");
        return -1;
    }

    /*******************************************父子进程*********************************** */
    /*******************************************父子进程*********************************** */
    pid_t pid = fork();
    if (pid > 0){                   // 父进程，与下位机通信，接受下位机的信息
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1){
            perror("socket err:");
            return -1;
        }

        struct sockaddr_in serverIn;
        serverIn.sin_addr.s_addr = inet_addr(ServerIp);
        serverIn.sin_family = AF_INET;
        serverIn.sin_port = htons(Port);

        int res = -1;                                                            // 接收属性值
        int reslen = sizeof(res);                                                // 接收属性的大小
        if ((getsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &res, &reslen)) == -1){ // 设置复用模式
            perror("getsockopt error");
            return -1;
        }

        if ((bind(sockfd, ((struct sockaddr *)&serverIn), sizeof(serverIn))) == -1){
            perror("bind error");
            return -1;
        }
        if ((listen(sockfd, 128)) == -1){
            perror("listen error");
            return -1;
        }

        struct sockaddr_in clientIn;
        socklen_t addrlen = sizeof(clientIn);

        int newfd = accept(sockfd, (struct sockaddr *)&clientIn, &addrlen);
        if (newfd == -1){
            perror("accept error:");
            exit(EXIT_FAILURE);
        }
        printf("[%s:%d]已连接\n", inet_ntoa(clientIn.sin_addr), ntohs(clientIn.sin_port));

    }
    else if (pid == 0){                              // 子进程，与QT通信
        //pthread_create(&tid1, NULL, PUB_func, NULL);
        MQTTconnect(&client, "tsdgfesg"); // 初始化,NULL随机生成clientID

        pthread_create(&tid2, NULL, SUB_func, NULL);
        // pthread_join(tid1, NULL);
        // pthread_join(tid2, NULL);
        while (1);
    }

    return 0;
}