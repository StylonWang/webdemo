/* 
 * File:   server.cpp
 * Author: root
 *
 * Created on 2014年6月26日, 下午 2:15
 */

#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "server.h"
#include "webapidefine.h"

#include <string>
#include <vector>

#ifdef MTRACE
#include <mcheck.h>
#endif

#include "webapi.h"
#include "protocol.h"

using namespace std;

// debug macro
#define LOG(fmt, arg...) do { fprintf(stderr, "[%s:%d] " fmt, __FILE__, __LINE__, ##arg); } while(0)

static request_dispatch_t g_dispatch[] = {
    
    { "GET",    "/test",                request_test},
    
    //2
    { "GET",    "/setled",               request_setled},
    { "POST",   "/setled",               request_setled},

};

static int g_server_should_stop = 0;
void signal_handler(int signo)
{
    printf("caught signal %d\n", signo);
    g_server_should_stop = 1;
}

int main(int argc, char **argv)
{
    webapi_t h;
    int ret;
    int port = 81;

    signal(SIGINT, signal_handler);

#ifdef MTRACE
    setenv("MALLOC_TRACE","memleak.log",1);
    mtrace();
#endif
    printf("argv[0] = %s\n",argv[0]);
    printf("argv[1] = %s\n",argv[1]);
    
    //char *aa;
    //aa = (char *)malloc(1);

	if(argv[1]!= NULL)
	{
		port = atoi(argv[1]);
	}

#if 1
    ret = webapi_start(&h, port, NULL, g_dispatch, sizeof(g_dispatch)/sizeof(g_dispatch[0]));
    if(ret) {
        fprintf(stderr, "fail to start webapi, error=%d\n", ret);
        exit(1);
    }
#endif

    LOG("webapi runs on %d port\n", port);

    while(!g_server_should_stop) 
    {
        sleep(1);

//#define _TEST
#ifdef _TEST
        sleep(20);
        g_server_should_stop = 1;
        LOG("stop webapi\n");
#endif
    }

    webapi_stop(h);

#ifdef MTRACE
    muntrace();
#endif
    return 0;
}
