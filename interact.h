
/* interact.h
 * HTTP服务器与客户端进行交互。
 * Copyright (c) 2024 Liu One  All rights reserved.
 *
 * 1. 客户端连接服务器时将在wait_connections函数中创建新的线程。
 * 2. 客户端会发送请求，服务器使用read_from函数读取请求，然后在process_connection函数中使用
 *    analyze_request函数解析请求。
 * 3. 解析后根据结果使用send_reply函数发送回复，其中会调用say_to函数。
 * 使用前请提前准备一个线程数组并设为空，然后依次将线程指针传入wait_connections。
 */

#ifndef _HTTPSERVER_INTERACT_H_
#define _HTTPSERVER_INTERACT_H_

#include <string.h>      // 字符串操作，解析客户端请求
#include <stdlib.h>      // 内存操作
#include <sys/socket.h>  // 网络操作
#include <pthread.h>     // 线程操作

#include "log.h"         // 日志记录

typedef struct {        // 一条信息
    char *key, *value;  // 一条信息有键和值
} information;

typedef struct {                   // 客户端发送的请求
    char *command, *site, *httpv;  // 命令、地址、HTTP版本号
    information infos[256];        // 其它的信息
    int info_length;               // 信息长度
} http_request;

void wait_connections(int, pthread_t *);
void *process_connection(void *);
int read_from(int, char *, int);
int say_to(int, char *);
http_request *analyze_request(char *);
void send_reply(int, http_request *);

void wait_connections(int socket, pthread_t *thread)
{
    /*
     * 阻塞直到客户端请求连接，并创建process_connection线程处理请求。
     */
    struct sockaddr_storage client_addr;
    socklen_t address_size = sizeof(client_addr);
    int connect_d = accept(   // 阻塞直到有客户端连接
        socket, (struct sockaddr *) &client_addr, &address_size);
    if (connect_d == -1)
        error("Unable to open secondary socket");
    int result = pthread_create(
        thread, NULL, process_connection, &connect_d);  // 创建线程处理客户端请求
    if (result == -1) {
        close(connect_d);
        error(
            "Failed to start the thread while connecting to Connection %i",
            connect_d);
    } else info("[Thread %lu] has been connected to Connection %i",
        *thread, connect_d);
}

void *process_connection(void *socket)
{
    /*
     * 读取客户端的请求并发送回复。
     */
    int connect_d = *(int *)socket;
    char *buf = (char *)malloc(10240);
    if (!buf || read_from(connect_d, buf, 10239) < 0) {  // 读取客户端请求
        error("[Connection %i] Unable to read the request", connect_d);
        goto failed;
    }
    http_request *req = analyze_request(buf);  // 解析请求
    if (!req) {
        error("[Connection %i] Unable to analyze the request", connect_d);
        goto failed;
    }
    info("[Connection %i] New request: %s %s",
        connect_d, req -> command, req -> site);
    send_reply(connect_d, req);  // 发送回复
    close(connect_d);
    info("[Connection %i] is closed", connect_d);
    free(req);
    failed:
        close(connect_d);
        return NULL;
}

int read_from(int socket, char *buf, int len)
{
    /*
     * 从客户端读取请求。
     */
    char *ptr = buf;
    int slen = len, readlen = recv(socket, ptr, slen, 0);
    while (readlen > 0 && ptr[readlen - 1] != '\n') {  // 循环读取字符
        ptr += readlen;
        slen -= readlen;
        readlen = recv(socket, ptr, slen, 0);
    }
    if (readlen < 0) return readlen;  // 发生错误
    if (readlen == 0) buf[0] = '\0';  // 空字符串
    else ptr[readlen - 1] = '\0';     // 将\n替换为\0
    return len - slen;
}

int say_to(int socket, char *msg)
{
    /*
     * 向客户端发送信息。
     */
    int result = send(socket, msg, strlen(msg), 0);
    if (result == -1)
        error("[Connection %i] Failed to send message", socket);
    return result;
}

http_request *analyze_request(char *buf)
{
    /*
     * 解析客户端的请求，返回http_request结构指针。
     */
    char *ptr, *lptr, *line;
    int count = 0;
    http_request *req = (http_request *)malloc(sizeof(http_request));
    // 下面三行代码处理第一行请求，包括三个要素：命令、地址、HTTP版本号
    if (!req || !(req -> command = strtok_r(buf, " ", &ptr))) goto failed;
    if (!(req -> site = strtok_r(NULL, " ", &ptr))) goto failed;
    if (!(req -> httpv = strtok_r(NULL, "\r", &ptr))) goto failed;
    for (line = strtok_r(NULL, "\r", &ptr);  // 循环处理下面每一行
            line; line = strtok_r(NULL, "\r", &ptr), ++count) {
        req -> infos[count].key = strtok_r(line, ": ", &lptr) + 1;
        req -> infos[count].value = strtok_r(NULL, "\r", &lptr);
        if (req -> infos[count].value) req -> infos[count].value++;
    }
    req -> info_length = count - 1;
    return req;
    failed:
        free(req);
        return NULL;
}

void send_reply(int socket, http_request *req)
{
    /*
     * 根据客户端的请求发送不同的回复。
     */
    if (say_to(socket, "HTTP/1.1 200 OK\r\n\r\n") == -1) return;
    if (say_to(socket, "Hello!\r\n") == -1) return;
}

#endif //!_HTTPSERVER_INTERACT_H_
