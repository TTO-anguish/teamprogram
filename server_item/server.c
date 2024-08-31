/**
 * @FilePath     : /home/ZF/bottom_program/server_item/server.c
 * @Description  :
 * @Author       : zongfei
 * @Version      : 0.0.1
 * @LastEditors  : zongfei
 * @LastEditTime : 2024-08-30 22:17:11
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
int StmFlag = 0;     //给下位机发送数据的标志，如果收到QT发来的信息，标志位置一，然后线程中发送数据
int msgid;
total_t SendToStm, recvFromStm;

void *PUB_func(void *buf)
{
    printf("进入  发送给QT的线程  成功\n");
    Mesgget_t msgbuf;
    corrToQt_t corr;
    char databuf[150];
    while(1){
        memset(&msgbuf, 0, sizeof(Mesgget_t));
        memset(&corr, 0, sizeof(corrToQt_t));

        msgrcv(msgid, &msgbuf, sizeof(Mesgget_t), 2, 0);
        corr.enviro = msgbuf.SendToBott.enviro;    //结构体给我搞复杂了，应该corrToQt_t里面直接放total_t的
        corr.fans = msgbuf.SendToBott.fans;        //这样就不用一个一个赋值了
        corr.time = msgbuf.SendToBott.time;
        corr.warn = msgbuf.SendToBott.warn;
        
        strcpy(databuf, corrToQt_to_json(&corr));
        mqtt_publish(PUB ,databuf, &client);
    }
}

void *SUB_func(void *buf)
{
    printf("进入  接受QT消息的线程  成功\n");
    corrToQt_t corr;
    Mesgget_t msgbuf;
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
            
            recvMQTTFlag = 0;//将标志位重置为0，可以再次接受MQTT的消息

            memset(data, 0, sizeof(data));

            //通过消息队列将数据发送给父进程的线程
            if(corr.sqlsearch == 0)  //如果QT发送的不是查找过往信息，那么为控制下位机信息
            {
                StmFlag = 1;       //置一，让父进程的线程函数能够发送消息
                msgbuf.type = 1;   
                msgbuf.SendToBott.enviro = corr.enviro;   //将QT发来的信息重新赋值
                msgbuf.SendToBott.fans = corr.fans;
                msgbuf.SendToBott.warn = corr.warn;
                msgbuf.SendToBott.time = corr.time;
            
                msgsnd(msgid, &msgbuf, sizeof(msgbuf.SendToBott), 0);  //发送给父进程的线程，让其发送给下位机
                memset(&msgbuf, 0, sizeof(Mesgget_t));
            }
            else{   //查找天气信息
                //  搜寻对应的数据库内容


                //将数据打包好发送给QT
            }
            
        }
        usleep(100000);
    }
}

//给下位机发送消息的线程函数
void* SerToStm_func(void* fd)  
{
    int newfd = *(int*)fd;    //得到socket的newfd
    Mesgget_t msgbuf;

    //消息从子进程的线程中而来,QT想要控制下位机的功能打开或者关闭
    while(1){

        memset(&msgbuf, 0, sizeof(Mesgget_t));
        msgrcv(msgid, &msgbuf, sizeof(Mesgget_t), 1, 0);
        SendToStm = msgbuf.SendToBott;
        if(StmFlag == 1) {
            send(newfd, &SendToStm, sizeof(total_t), 0);
            StmFlag = 0;
        }
    }
}

int main(int argc, const char *argv[])
{
    pthread_t tid1, tid2;
    pthread_t SerToStm;

    key_t key = ftok("./ipc", 1);
    if (key == -1){
        perror("ftok err");
        return -1;
    }
    if ((msgid = msgget(key, IPC_CREAT | 0666)) == -1){
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

        pthread_create(&SerToStm, NULL, SerToStm_func, &newfd);  //创建线程，用于给下位机发送消息
        pthread_detach(SerToStm);

        /****************接受下位机信息************************/
        Mesgget_t msgbuf;
        while (1)
        {
            memset(&recvFromStm, 0, sizeof(total_t));
            int ret = recv(newfd, &recvFromStm, sizeof(total_t), 0);
            if(ret == 0){
                printf("下位机断开连接!\n");
                return -1;
            }
            //将下位机消息发送给子进程的发布线程
            
            msgbuf.type = 2;
            msgbuf.SendToBott = recvFromStm;
            msgsnd(msgid, &msgbuf, sizeof(msgbuf), 0);

        }
        
    }
    else if (pid == 0){                              // 子进程，与QT通信
        
        MQTTconnect(&client, "tsdgfesg"); // 初始化,NULL随机生成clientID
        pthread_create(&tid1, NULL, PUB_func, NULL);
        pthread_create(&tid2, NULL, SUB_func, NULL);
        // pthread_join(tid1, NULL);
        // pthread_join(tid2, NULL);
        while (1);
    }

    return 0;
}