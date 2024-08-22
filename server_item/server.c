/**
 * @FilePath     : /home/ZF/bottom_program/server_item/server.c
 * @Description  :  
 * @Author       : zongfei
 * @Version      : 0.0.1
 * @LastEditors  : zongfei
 * @LastEditTime : 2024-08-22 19:23:34
 * @Copyright    : ZFCompany AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2024.
 * @Email: 15056030055@163.com
**/
#include <myhead.h>
#include <cjson/cJSON.h>
#include <MQTTClient.h>
#include <sqlite3.h>
#include "transducer_struct.h"
#define ServerIp "172.19.113.181"
#define Port 8888
#define sqlname "Client.db"

int main(int argc,const char * argv[])
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1){
        perror("socket err:");
        return -1;
    }

    struct sockaddr_in serverIn;
    serverIn.sin_addr.s_addr = inet_addr(ServerIp);
    serverIn.sin_family = AF_INET;
    serverIn.sin_port = htons(Port);

    int res = -1;              //接收属性值
    int reslen = sizeof(res);  //接收属性的大小
    if( ( getsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &res, &reslen) ) ==-1)  //设置复用模式
    {
        perror("getsockopt error");
        return -1;
    }

    if( ( bind(sockfd, ((struct sockaddr*)&serverIn), sizeof(serverIn)) ) == -1){
        perror("bind error");
        return -1;
    }
    if( ( listen(sockfd, 128) ) == -1){
        perror("listen error");
        return -1;
    }

    struct sockaddr_in clientIn;
    socklen_t addrlen = sizeof(clientIn);

    int newfd = accept(sockfd, (struct sockaddr*)&clientIn, &addrlen);
    if (newfd == -1){
        perror("accept error:");
        exit(EXIT_FAILURE);
    }
    printf("[%s:%d]已连接\n", inet_ntoa(clientIn.sin_addr), ntohs(clientIn.sin_port));
    total_t total;
    while (1)
    {   
        memset(&total, 0, sizeof(total_t));
        recv(newfd, &total, sizeof(total_t), 0);
        printf("total.enviro.hum %f\n", total.enviro.hum);
        printf("total.enviro.NH3 %f\n", total.enviro.NH3);
        printf("total.enviro.sunshine %f\n", total.enviro.sunshine);
        printf("total.enviro.tem %f\n", total.enviro.tem);
    }
    


    return 0;
}