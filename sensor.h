#ifndef SENSOR_H
#define SENSOR_H
#include <QDateTime>
#include <QTimer>
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

typedef struct corrToQt
{
    envir_t   enviro;
    warn_t      warn;
    time_t      time;
    int         fans;//风扇  0：关  1：慢  2：快
    int    sqlsearch;//
}corrToQt_t;
#endif // SENSOR_H
