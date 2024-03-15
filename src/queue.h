﻿#ifndef _OS_LOG_QUEUE_H_
#define _OS_LOG_QUEUE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct _os_queue_t os_queue_t;
typedef struct _os_queue_node_t os_queue_node_t;

/*
* os_queue_create
* @brief  创建队列
* @param  size     节点大小
* @return 队列指针或者为NULL
*/
os_queue_t * os_queue_create(size_t size);

/*
* os_queue_destroy
* @brief  销毁队列
* @param  oq    指向队列指针的指针
*/
void os_queue_destroy(os_queue_t ** oq);

/*
* os_queue_clear
* @brief  清空队列
* @param  oq    队列指针
*/
void os_queue_clear(os_queue_t * oq);

/*
* os_queue_empty
* @brief  判断队列是否为空
* @param  oq    队列指针
* @return true--成功 false--失败
*/
bool os_queue_empty(os_queue_t * oq);

/*
* os_queue_push
* @brief  插入数据
* @param  oq    队列指针
* @param  data  数据指针
* @return true--成功 false--失败
*/
bool os_queue_push(os_queue_t * oq, void * data);

/*
* os_queue_pop
* @brief  删除队列头
* @param  oq     队列指针
*/
void os_queue_pop(os_queue_t * oq);

/*
* os_queue_front
* @brief  获取队列头
* @param  os_queue  队列指针
* @return 头结点指针或者NULL
*/
os_queue_node_t * os_queue_front(os_queue_t * oq);

/*
* os_queue_getdata
* @brief  获取节点数据
* @param  node    节点指针
* @return 数据指针
*/
void * os_queue_getdata(const os_queue_node_t * node);

#endif
