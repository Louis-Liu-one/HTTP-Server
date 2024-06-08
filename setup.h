
/* setup.h
 * 为服务器与客户端交互作准备工作。包含打开服务器、绑定端口、监听等函数。
 * Copyright (c) 2024 Liu One  All rights reserved.
 *
 * map_shutdown_handler用于设置处理退出信号（SIGINT）的函数，应当首先调用。
 * open_listener_socket、bind_to_port、start_listening是用于准备通信的函数，
 * 详情请参见函数文档。
 */

#ifndef _HTTPSERVER_SETUP_H_
#define _HTTPSERVER_SETUP_H_

#include <signal.h>  // Ctrl-C（退出）信号的处理
#include <unistd.h>  // 这三个头文件用于处理网络操作
#include <arpa/inet.h>
#include <sys/socket.h>

#include "log.h"     // 日志记录

int map_shutdown_handler(void (*)(int));  // 设置退出信号处理函数
int open_listener_socket();               // 创建服务器
void bind_to_port(int, int);              // 绑定到端口
void start_listening(int, int);           // 开始监听

int map_shutdown_handler(void (*shutdown_handler)(int))
{
    /*
     * 设置退出信号处理函数。该函数必须接受一个int（即退出的信号）且没有返回值。
     * 若出现问题，则返回-1。
     */
    struct sigaction action;
    action.sa_handler = shutdown_handler;     // 设置函数
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    return sigaction(SIGINT, &action, NULL);  // 绑定到SIGINT
}

int open_listener_socket()
{
    /*
     * 创建服务器并返回。
     */ 
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) failed("Unable to create the socket");  // 失败
    return sock;
}

void bind_to_port(int socket, int port)
{
    /*
     * 设置端口重用选项，并将服务器绑定到端口port。
     */
    struct sockaddr_in name;
    name.sin_family = PF_INET;
    name.sin_port = (in_port_t)htons(port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    int reuse = 1;
    if (setsockopt(                                // 端口重用
            socket, SOL_SOCKET, SO_REUSEADDR,
            (char *) &reuse, sizeof(int)) == -1)
        failed("Unable to set the reuse option");
    info("Reuse option: OPEN");
    if (bind(socket, (struct sockaddr *) &name, sizeof(name)) == -1)
        failed("Unable to bind to socket"); // 绑定到端口
    info("Bind to port %i", port);
}

void start_listening(int socket, int quelen)
{
    /*
     * 开始等待客户端连接。
     */
    if (listen(socket, quelen) == -1) failed("Unable to listen");
    info("Waiting for connections...");
}

#endif  // !_HTTPSERVER_SETUP_H_
