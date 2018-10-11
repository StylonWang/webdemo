/* 
 * File:   protocol.h
 * Author: Aric
 *
 * Created on 2014年7月9日, 上午 9:41
 */
#include "webapi.h"

#ifndef PROTOCOL_H
#define	PROTOCOL_H

#ifdef __cplusplus
extern "C"{
#endif

char *request_test(webapi_t t, webapi_data_t *v);

//2
char *request_setled(webapi_t t, webapi_data_t *v);              

#ifdef __cplusplus
} 
#endif

#endif	/* PROTOCOL_H */

