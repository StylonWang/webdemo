/* 
 * File:   webapidefine.h
 * Author: Aric
 *
 * Created on 2014年6月27日, 上午 9:52
 */

#ifndef WEBAPIDEFINE_H
#define	WEBAPIDEFINE_H

typedef enum _WEB_RESULT        //return to web error code
{
    WEB_SUCCESS =               0,      //success
    WEB_MISSING_PARAM =        -1,      //no complete param
    WEB_LOGIN_FAIL =           -2,      //username or password error
    WEB_SESSION_ERROR =        -3,      //session error
    WEB_NO_DEFINE =            -4
}WEB_RESULT;

#define HTTP_USERNAME   "a123456789"
#define HTTP_PASSWORD   "b987654321"

#define RANDOM_MAX       RAND_MAX/2
#define WEB_RETURN_BODY_LEN    2048
#define WEB_JSON_BUFFER_LEN     256
#define WEB_PARAM_LEN           256

//#define WEB_RETURN_JSON_BODY                "{\n \"result\": %d, \n \"session\": %d}\n"
#define WEB_RETURN_JSON_BODY          "{\n \"result\": %d,\n \"invalidArg\": \"%s\"\n}\n"

#define WEB_USERNAME    "userName"
#define WEB_PASSWORD    "password"
#define WEB_SESSION      "session"
#define WEB_LANGUAGE    "language"

#define WEB_DEFAULT_USERNAME        "admin"
#define WEB_DEFAULT_PASSWORD    "avermedia"

#define WEB_AAA    "aaa"
#define WEB_AA     "aa"



#endif	/* WEBAPIDEFINE_H */

