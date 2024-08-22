/**
 * @FilePath     : /home/ZF/TCP/weather.h
 * @Description  : 关于天气的结构体
 * @Author       : zongfei
 * @Version      : 0.0.1
 * @LastEditors  : zongfei
 * @LastEditTime : 2024-08-18 16:12:31
 * @Copyright    : ZFCompany AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2024.
 * @Email: 15056030055@163.com
**/
#ifndef _WEATHER_H_
#define _WEATHER_H_


typedef struct WEATHER
{
    char   temperature[10];//温度
    char          name[20]; //地址
    char       country[10];//国家
    char        weather[6];//天气现象
    char       pressure[6];//大气压
    char       humidity[4];//湿度
    char     visibility[6]; //能见度
    char wind_direction[6];//风向
    char     wind_speed[8];//风速
    char     wind_scale[2];//风力等级
}weather_t;

typedef struct CLIENT
{
    char     name[20];  //用户名称
    char  password[7];  //用户密码
    int          flag;  //登录消息：1  注册消息：0
    char position[15];  //要查找的地址
    int         exist;  //人员是否存在
    int         newfd;  //连接到用户的新的套接字
}client_t;

typedef struct TOTAL
{
    weather_t   weath;
    client_t    total_cli;
}total_t;

#endif // !_WEATHER_H_