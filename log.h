
/* log.h
 * 服务器的日志记录。
 * Copyright (c) 2024 Liu One  All rights reserved.
 *
 * 记录日志的函数共有：failed、error、warning、question、info。
 * set_exit_func用于设置failed在退出前调用的收尾函数，now用于获取当前时间。
 * 所有记录日志的函数，都可以以类似printf的形式使用，因此可以接受任意参数，如：
 * error("[Thread %lu]Unable to open secondary socket", *thread);
 * failed("Connection %i failed to start thread", connection);
 */

#ifndef _HTTPSERVER_LOG_H_
#define _HTTPSERVER_LOG_H_

#include <stdio.h>    // 标准输入输出
#include <stdlib.h>   // 内存操作
#include <string.h>   // 字符串操作
#include <stdarg.h>   // 给函数传入多个参数
#include <time.h>     // 获取时间
#include <pthread.h>  // 线程锁
#include <signal.h>   // 信号处理

// 记录日志的函数可能会在不同的线程中调用，因此需要用输出锁避免错乱
pthread_mutex_t output_lock = PTHREAD_MUTEX_INITIALIZER;
char *now();                        // 获取当前时间
void set_exit_func(int (*)());      // 设置failed在退出前调用的函数
void failed(const char *, ...);     // 失败（退出）
void error(const char *, ...);      // 错误（不会退出）
void warning(const char *, ...);    // 警告
char *question(const char *, ...);  // 询问，返回输入内容
void info(const char *, ...);       // 信息
int (*exit_func)();                 // failed在退出前调用的函数，需要先设置

char *now()
{
    /*
     * 获取现在时间，返回形如“Thu Mar 14 15:09:26 2024”的字符串。
     */
    time_t t;
    time(&t);
    char *timenow = asctime(localtime(&t));
    timenow[strlen(timenow) - 1] = '\0';
    return timenow;
}

void set_exit_func(int (*func)())
{
    /*
     * 设置exit_func。详情请参见failed函数。
     */
    exit_func = func;
}

void failed(const char *format, ...)
{
    /*
     * 出现严重错误，输出日志，同时以exit_func()为返回值退出。用红色高亮。
     * 若exit_func()为0，则不会退出。你可以在exit_func()中执行一些退出前的操作。
     * 请提前用set_exit_func设置exit_func。
     */
    va_list ap;
    va_start(ap, format);
    char *new_format = (char *)malloc(1024);
    sprintf(
        new_format,
        "\33[1;31;101m  FAILED\33[0;1;31m [%s] \33[91m  %s\33[0m\n",
        now(), format);
    pthread_mutex_lock(&output_lock);
    vprintf(new_format, ap);
    pthread_mutex_unlock(&output_lock);
    va_end(ap);
    free(new_format);
    int exit_status = exit_func();
    if (exit_status != 0) exit(exit_status);
}

void error(const char *format, ...)
{
    /*
     * 出现错误，仅输出错误信息，不会退出。用红色高亮。
     */
    va_list ap;
    va_start(ap, format);
    char *new_format = (char *)malloc(1024);
    sprintf(
        new_format,
        "\33[1;31;101m   ERROR\33[0;1;31m [%s] \33[91m  %s\33[0m\n",
        now(), format);
    pthread_mutex_lock(&output_lock);
    vprintf(new_format, ap);
    pthread_mutex_unlock(&output_lock);
    va_end(ap);
    free(new_format);
}

void warning(const char *format, ...)
{
    /*
     * 警告。用黄色高亮。
     */
    va_list ap;
    va_start(ap, format);
    char *new_format = (char *)malloc(1024);
    sprintf(
        new_format,
        "\33[1;33;103m WARNING\33[0;1;33m [%s] \33[93m  %s\33[0m\n",
        now(), format);
    pthread_mutex_lock(&output_lock);
    vprintf(new_format, ap);
    pthread_mutex_unlock(&output_lock);
    va_end(ap);
    free(new_format);
}

char *question(const char *format, ...)
{
    /*
     * 询问。用洋红色高亮。
     */
    va_list ap;
    va_start(ap, format);
    char *new_format = (char *)malloc(1024);
    sprintf(
        new_format,
        "\33[1;35;105mQUESTION\33[0;1;35m [%s] \33[95m  %s\33[0m",
        now(), format);
    pthread_mutex_lock(&output_lock);
    vprintf(new_format, ap);
    char *answer = (char *)malloc(32);
    fgets(answer, 31, stdin);
    pthread_mutex_unlock(&output_lock);
    answer[strlen(answer) - 1] = '\0';
    va_end(ap);
    free(new_format);
    return answer;
}

void info(const char *format, ...)
{
    /*
     * 信息。用蓝色高亮。
     */
    va_list ap;
    va_start(ap, format);
    char *new_format = (char *)malloc(1024);
    sprintf(
        new_format,
        "\33[1;36;106m    INFO\33[0;1;36m [%s] \33[96m  %s\33[0m\n",
        now(), format);
    pthread_mutex_lock(&output_lock);
    vprintf(new_format, ap);
    pthread_mutex_unlock(&output_lock);
    va_end(ap);
    free(new_format);
}

#endif  // !_HTTPSERVER_LOG_H_
