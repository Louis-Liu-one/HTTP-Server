
/* server.c
 * HTTP服务器。
 * Copyright(c) 2024 Liu One  All rights reserved.
 *
 * 简单的HTTP服务器，可以响应客户端的连接。
 */

#include <stdio.h>       // 标准输入输出
#include <stdlib.h>      // exit函数与内存操作
#include <signal.h>      // 信号处理
#include <unistd.h>      // 这两个头文件是网络操作
#include <sys/socket.h>
#include <pthread.h>     // 线程操作

#include "log.h"         // 日志记录
#include "setup.h"       // 初始化服务器
#include "interact.h"    // 与客户端的交互

int listener_d;              // 服务器
pthread_t threads[128];      // 线程池
int shutdown_func();         // 退出函数
void shutdown_handler(int);  // 按下Ctrl-C时的退出函数

int shutdown_func()
{
    /*
     * 为服务器的关闭做收尾工作，但不退出程序。返回1。
     */
    if (listener_d) close(listener_d);  // 关闭服务器
    void *result;
    for (int i = 0; i < 128; ++i) {
        info("Join the thread %lu", threads[i]);
        if (pthread_join(threads[i], &result) == -1)  // 等待线程结束
            error("Unable to join Thread %lu", threads[i]);
    }
    warning("Server was shut down");
    warning("Closed...");
    return 1;
}

void shutdown_handler(int sig)
{
    /*
     * 按下Ctrl-C时调用。将询问是否关闭，若回答为是，则做收尾工作并退出程序。
     */
    if (sig == SIGINT) {
        printf("\b\b");
        char answer = question(
            "Are you sure to shutdown the server? [y/N] ")[0];
        if (answer == 'Y' || answer == 'y') exit(shutdown_func() - 1);
    }
}

int main()
{
    set_exit_func(shutdown_func);                                // 设置收尾函数
    if (map_shutdown_handler(shutdown_handler) == -1) return 1;  // 设置信号响应函数
    listener_d = open_listener_socket();                         // 打开服务器
    bind_to_port(listener_d, 8080);                              // 绑定端口
    start_listening(listener_d, 16);                             // 开始监听
    while (1)
        for (int i = 0; i < 128; ++i)
            wait_connections(listener_d, threads + i);           // 阻塞直到有客户端连接
    return 0;
}
