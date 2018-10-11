#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "protocol.h"
#include "webapidefine.h"


#define LOG(fmt, arg...) do { fprintf(stderr, "[%s:%d] " fmt, __FILE__, __LINE__, ##arg); } while(0)


const char *fn_errorCodeStr(WEB_RESULT error_code)
{
    return "";
}

char *request_test(webapi_t t, webapi_data_t *v)
{
    LOG("%s()\n", __func__);
    static int session = 0;

    LOG("v->method = %s\n",v->method);
    LOG("v->content_type = %s\n",v->content_type);
    LOG("v->url = %s\n",v->url);
    LOG("v->postdata = %s\n",v->postdata);
    
    WEB_RESULT result = WEB_NO_DEFINE;
    
    char szUserName[WEB_PARAM_LEN] = {0};
    char szPassword[WEB_PARAM_LEN] = {0};
    char aaa[WEB_PARAM_LEN] = {0};
    char aa[WEB_PARAM_LEN] = {0};
    
    (WEBAPI_SUCCESS == webapi_get_request_value(t, WEB_USERNAME, szUserName, sizeof(szUserName) ))? result = result:result = WEB_MISSING_PARAM;
    LOG("result = %d\n", result);
    (WEBAPI_SUCCESS == webapi_get_request_value(t, WEB_PASSWORD, szPassword, sizeof(szPassword) ))? result = result:result = WEB_MISSING_PARAM;
    LOG("result = %d\n", result);

    (WEBAPI_SUCCESS == webapi_get_request_value(t, WEB_AAA, aaa, sizeof(aaa) ))? result = result:result = WEB_MISSING_PARAM;
    LOG("result = %d\n", result);
    (WEBAPI_SUCCESS == webapi_get_request_value(t, WEB_AA, aa, sizeof(aa) ))? result = result:result = WEB_MISSING_PARAM;
    LOG("result = %d\n", result);
    

    LOG("username = %s\n",szUserName);
    LOG("password = %s\n",szPassword);
    LOG("aaa = %s\n",aaa);
    LOG("aa = %s\n",aa);

    char *reply = (char *)malloc(WEB_RETURN_BODY_LEN);
    snprintf(reply, WEB_RETURN_BODY_LEN, "{\"result\": 0, \"session\": %d}\n", session++);
    
    LOG("request_test end \n");
    
    return reply;
}

char *request_setled(webapi_t t, webapi_data_t *v)
{
    LOG("%s()\n", __func__);
    char *reply = (char *)malloc(WEB_RETURN_BODY_LEN);

    WEB_RESULT result = WEB_SUCCESS;
    
    char value[WEB_PARAM_LEN] = {0};
    
    if(strcmp(v->method, "POST") == 0 )
    LOG("v->postdata = %s\n", v->postdata);

    //check web param is missing or not
    result = WEB_SUCCESS;

    (WEBAPI_SUCCESS == webapi_get_request_value(t, "v", value, sizeof(value) ))? result = result:result = WEB_MISSING_PARAM;

    LOG("value=%s\n",value);
    
    if(result != WEB_SUCCESS)
    {
        LOG("missing the param\n");
        snprintf(reply, WEB_RETURN_BODY_LEN, WEB_RETURN_JSON_BODY, result, fn_errorCodeStr(result));
        return reply;
    }

    // open led driver to set its value       
    static int toggle_value = 0;
    char buf[8];
    int fd = open("/sys/class/leds/green:ph24:led1/brightness", O_WRONLY);
    if(fd<0) {
        LOG("unable to open led brightness: %s\n", strerror(errno));

    }

    switch(atoi(value)) {
    case 0:
        snprintf(buf, sizeof(buf), "0");
        write(fd, buf, strlen(buf));
        break;

    case 1:
        snprintf(buf, sizeof(buf), "1");
        write(fd, buf, strlen(buf));
        break;

    case 2:
    default:
        snprintf(buf, sizeof(buf), "%d", toggle_value);
        toggle_value ^= 1; // invert by XOR
        write(fd, buf, strlen(buf));
        break;
    }

    result = WEB_SUCCESS;
    snprintf(reply, WEB_RETURN_BODY_LEN, WEB_RETURN_JSON_BODY, result, "");
    return reply;
}

