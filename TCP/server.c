/**
 * @FilePath     : /home/ZF/TCP/server.c
 * @Description  : 服务器端，接收天气数据请求，发送天气数据给客户端
 * @Author       : zongfei
 * @Version      : 0.0.1
 * @LastEditors  : zongfei
 * @LastEditTime : 2024-08-18 16:26:51
 * @Copyright    : ZFCompany AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2024.
 * @Email: 15056030055@163.com
**/
#include <myhead.h>
#include <cjson/cJSON.h>
#include <curl/curl.h>
#include "pthread_pool.h"
#include "weather.h"
#include "func.h"

#define ServerIp "172.19.113.181"
#define Port 8888
#define sqlname "Client.db"
#define tabname "usrinfo"
#define namer "name char"
#define passwd "passwd char"
#define ISEXIT 1

sqlite3* ppDb;
int newfd[128];

void* thread_func(void* data){
    int th_newfd = *(int*)data;
    total_t total, tot_data;
    char data_buf[600] = {0}, recvdata[600] = "";
    char recv_data[600] = {0};
    printf("socketfd = %d\n", th_newfd);
    
    int ret = recv(th_newfd, &recv_data, sizeof(recv_data), 0); //接收消息(接收登录消息，没有其他东西)
    if(ret == 0){
        printf("客户端断开连接\n");
        close(th_newfd); 
    }
    printf("data_buf = %s\n", recv_data);
    
    
    //将数据转化为total
    json_to_total(recv_data, &total);
    
    client_t pth_client = total.total_cli;
    //创建一个结构体，每连接一个新用户，从JSON中获取天气信息，然后发送给客户端

    printf("pth_client->name = %s\n", pth_client.name);
    printf("pth_client->passwd = %s\n", pth_client.password);

    //判断
    while(1){
        pth_client = total.total_cli;
        int num = RegistOrSign(ppDb, pth_client, tabname);
        if(num == -1){
            total.total_cli.exist = 0;  //人员不存在
            char* buf = total_to_json(&total);
            send(th_newfd, buf, sizeof(buf), 0);
        }
        else{  //人员存在
            total.total_cli.exist = 1;
            char* buf = total_to_json(&total);
            
            printf("%s:服务器发送的数据\n", buf);
            
            char cli_data[600] = {};
            strcpy(cli_data, buf);
            send(th_newfd, cli_data, sizeof(cli_data), 0);//根据标志跳转页面
            break;
        }
        memset(data_buf, 0, sizeof(data_buf));
        recv(th_newfd, data_buf, sizeof(data_buf), 0);  //不成功就继续接收
        if(json_to_total(data_buf,&total) == -1) return NULL;
    }
      
    //接收用户消息
    while (1){
        CURL* curl;
        memset(&tot_data, 0, sizeof(tot_data));
        int ret = recv(th_newfd, recvdata, sizeof(recvdata), 0);  //接收要查找的地址信息
        if(ret == -1){
            perror("pthread recv err:");
            return NULL;
        }
        ret = json_to_total(recvdata, &tot_data);  //将字符型转为json在转为total_t
        if(ret == -1){
            printf("线程第二个while的json_to_total err\n");
            return NULL;
        }
        //GET URL
        if(tot_data.total_cli.position[strlen(tot_data.total_cli.position) - 1] == '\n')
            tot_data.total_cli.position[strlen(tot_data.total_cli.position) - 1] = '\0'; 
        char* data = get_URL(&curl, tot_data.total_cli.position);

        printf("get_URL char* = %s\n", data);

        //解析JSON将信息放入tot_data.weath中
        weather_t weather;
        // cJSON *json = cJSON_Parse(data);
        // json_to_weather(json, &weather);  //转换为weather_t
        ret = parse_weather_json(data, &weather);
        if(ret == -1){
            printf("parse_weather_json err\n");
            return NULL;
        }

        tot_data.weath = weather;
        tot_data.total_cli = total.total_cli;
        //发送
        char* mybuf = total_to_json(&tot_data);
        char total_weath_data[600] = {0};
        strcpy(total_weath_data, mybuf);
        printf("total_weath_data = %s\n", total_weath_data);

        ret = send(th_newfd, total_weath_data, sizeof(total_weath_data), 0);
        if(ret == -1){
            perror("Pthread send err:");
            return NULL;
        }
        free(data);
    }
    
}

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
    
    //创建/打开数据库
    ppDb = OpenDataBase(sqlname, tabname, "name char, passwd char");
    if(ppDb == NULL){
        printf("数据库打开失败\n");
        return -1;
    }

    if( ( tpool_create(10) ) != 0){          //创建10个线程
        printf("%s failed ", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    cJSON json_data;
    int i = 0;
    while (1){
        newfd[i] = accept(sockfd, (struct sockaddr*)&clientIn, &addrlen);
        if (newfd[i] == -1){
            perror("accept error:");
            exit(EXIT_FAILURE);
        }
        printf("fd[i] = %d\n", newfd[i]);
        printf("[%s:%d]\n", inet_ntoa(clientIn.sin_addr), ntohs(clientIn.sin_port));
        
        tpool_add_work(thread_func, (void*)(newfd + i));//向线程池中加入任务    
        i++;   
    }

    sqlite3_close(ppDb);
    tpool_destory();
    return 0;
}
