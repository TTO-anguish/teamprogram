/**
 * @FilePath     : /home/ZF/TCP/pthread_pool.h
 * @Description  :  
 * @Author       : zongfei
 * @Version      : 0.0.1
 * @LastEditors  : zongfei
 * @LastEditTime : 2024-08-13 20:50:05
 * @Copyright    : ZFCompany AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2024.
 * @Email: 15056030055@163.com
**/
#ifndef _PTHREAD_POOL_H_
#define _PTHREAD_POOL_H_

#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

/*要执行的任务链表*/
typedef struct tpool_work
{
    void* (*routine)(void*);     //任务函数
    void* arg;                  //传入任务函数的参数
    struct tpool_work *next;
}tpool_work_t;

typedef struct tpool
{
    int shutdown;               /*线程是否销毁*/
    int max_thr_num;            /*最大线程数*/
    pthread_t* thr_id;          /*线程ID数组*/
    tpool_work_t *queue_head;   /*线程链表*/
    pthread_mutex_t queue_lock; /*互斥锁*/
    pthread_cond_t queue_ready;
    
}tpool_t;

/**
 * @brief        : 创建线程池
 * @param         {int} max_thr_num:最大线程数
 * @return        {*}成功返回0
**/
int tpool_create(int max_thr_num);

/**
 * @brief        : 销毁线程池
 * @return        {*}
**/
void tpool_destory();

/**
 * @brief        : 向线程池中添加任务
 * @param         {routine}：任务函数指针
 * @param         {void*} arg:任务函数参数
 * @return        {*}
**/
int tpool_add_work(void*(*routine)(void*), void* arg);

#endif // !_PTHREAD_POOL_H_
