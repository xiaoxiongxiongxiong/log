#include "queue.h"
#include <string.h>
#include <malloc.h>

struct _os_queue_node_t
{
    struct _os_queue_node_t * next;
    char data[0];
};

struct _os_queue_t
{
    size_t cnt;   // 节点个数
    size_t size;  // 节点大小
    struct _os_queue_node_t * head;  // 队列头
    struct _os_queue_node_t * tail;  // 队列尾
};

os_queue_t * os_queue_create(const size_t size)
{
    os_queue_t * oq = (os_queue_t *)malloc(sizeof(os_queue_t));
    if (NULL != oq)
    {
        oq->cnt = 0ul;
        oq->size = size;
        oq->head = NULL;
        oq->tail = NULL;
    }
    return oq;
}

void os_queue_destroy(os_queue_t ** oq)
{
    if (NULL != oq && NULL != *oq)
    {
        os_queue_clear(*oq);
        free(*oq);
        *oq = NULL;
    }
}

void os_queue_clear(os_queue_t * oq)
{
    if (NULL == oq)
        return;

    while (NULL != oq->head)
    {
        os_queue_node_t * head = oq->head;
        oq->head = oq->head->next;
        free(head);
        head = NULL;
        --oq->cnt;
    }
    oq->tail = NULL;
}

bool os_queue_empty(os_queue_t * oq)
{
    if (NULL == oq)
        return true;
    return 0ul == oq->cnt;
}

bool os_queue_push(os_queue_t * oq, void * data)
{
    if (NULL == oq || NULL == data)
        return false;

    os_queue_node_t * node = (os_queue_node_t *)malloc(sizeof(os_queue_node_t) + oq->cnt);
    if (NULL == node)
        return false;

    node->next = NULL;
    memcpy(node->data, data, oq->size);
    if (0ul == oq->cnt)
    {
        oq->head = node;
        oq->tail = node;
    }
    else
    {
        oq->tail->next = node;
        oq->tail = node;
    }
    ++oq->cnt;

    return true;
}

void os_queue_pop(os_queue_t * oq)
{
    if (NULL == oq || 0ul == oq->cnt)
        return;

    os_queue_node_t * head = oq->head;
    oq->head = oq->head->next;
    free(head);
    --oq->cnt;
    if (0ul == oq->cnt)
    {
        oq->head = NULL;
        oq->tail = NULL;
    }
}

os_queue_node_t * os_queue_front(os_queue_t * oq)
{
    return oq ? oq->head : NULL;
}

void * os_queue_getdata(const os_queue_node_t * node)
{
    return node ? (void *)node->data : NULL;
}
