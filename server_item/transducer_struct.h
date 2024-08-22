/**
 * @FilePath     : /home/ZF/bottom_program/server_item/transducer_struct.h
 * @Description  :  
 * @Author       : CMH,ZF,ZY,SSS
 * @Version      : 0.0.1
 * @LastEditors  : zongfei
 * @LastEditTime : 2024-08-22 16:31:27
**/
#include <time.h>

typedef struct environment
{
    float      hum;      //湿 度
    float      tem;      //温 度
    float sunshine;      //光照强度
    float      NH3;      //氨气浓度
}envir_t;

typedef struct WARN
{
    int flag_NH3;   //NH3超标则置为1
    int flag_tem;   //温度过高则置为1
}warn_t;

typedef struct total
{
    envir_t   enviro;
    warn_t      warn;
    time_t      time;
    int         fans;      //风扇  0：关  1：慢  2：快                      
}total_t;
