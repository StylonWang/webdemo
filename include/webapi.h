#ifndef __WEBAPI_H__
#define __WEBAPI_H__

// TODO:
//
// 1. design interface to generate http reply
// 2. add interface: get HTTP variables and set response headers
// 3. access to POST DATA not verified
// 4. multiple request by multiple threads not implemented
// 5. integration into cm_main not discussed
// 6. Doxygen documentation!

// handle to webapi
typedef void * webapi_t;

// handle to HTTP reply
//typedef void * http_reply_t;

// call back function to handle web api requests

// @ret Must be string from a malloc(or equivelant) or NULL if critical error happens.

typedef struct webapi_data_t
{
    const char *url;
    const char *method;
    char content_type[64];
    char *postdata;
    int content_length;
}webapi_data_t;

//typedef char* (*webapi_callback_t) (webapi_t t, const char *method, void *data, const char *request, const char *postdata, int postdata_len, const char *content_type); 
typedef char* (*webapi_callback_t) (webapi_t t, webapi_data_t *v);


typedef struct request_dispatch_t {
    const char *method; // "GET" or "POST"
    const char *request; 
    webapi_callback_t callback; // function to handle this request
} request_dispatch_t;

#ifdef __cplusplus
extern "C"{
#endif
    
typedef enum _WEBAPI_RESULT     //webapi error code
{
    WEBAPI_SUCCESS =             0,     //success
    WEBAPI_PARAM_NULL_POINT =   -1,     //there are NULL point in params
    WEBAPI_NO_PARAM =           -2,     //missing parameter value
    WEBAPI_NO_VALUE =           -3,     //cant get the query value
    WEBAPI_INTERNAL_ERROR =     -4,     //internal process error
    WEBAPI_ENOMEM         =    -12,     //out of memory use ENOMEM
    WEBAPI_NO_DEFINE =          -5
}WEBAPI_RESULT;

WEBAPI_RESULT webapi_start(webapi_t *pt, int port, void *data, request_dispatch_t *dispatch, int num_dispatch);
WEBAPI_RESULT webapi_stop(webapi_t t);
WEBAPI_RESULT webapi_get_request_value(webapi_t t, const char *key,char *value, const unsigned int length);
int safeAtoi(const char *pPtr);

#ifdef __cplusplus
} 
#endif

#endif //__WEBAPI_H__
