#include <microhttpd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "webapi.h"
#include "webapidefine.h"

#include <string>
#include <vector>

using namespace std;

// debug macro
#define LOG(fmt, arg...) do { fprintf(stderr, "[%s:%d] " fmt, __FILE__, __LINE__, ##arg); } while(0)

// internal data structure used through-out the life time of this API instnace
class CHttpPostParam
{
public:
	CHttpPostParam();
	~CHttpPostParam();

	char *m_pszKey;
	char *m_pszValue;
};

enum HTTP_CONTENT_TYPE
{
    CONTENT_TYPE_UNKNOWN,
    CONTENT_TYPE_MULTIPART,
    CONTENT_TYPE_TEXT_XML,
};

// internal data structure used per connection/request
typedef struct webapi_request_t
{
    const char *method;
    struct MHD_Connection *connection;
    int expected_data_length;
    char content_type[64];
    char *data_buffer;
    int last_data;

    int dispatch_index;
    //std::vector<CHttpPostParam *> m_vpQuery;

} webapi_request_t;

std::vector<CHttpPostParam *> m_vpQuery;

typedef struct webapi_struct_t 
{
    struct MHD_Daemon *d;
    request_dispatch_t *dispatch;
    int num_dispatch;
    void *private_data;
    webapi_request_t *webapi_request;

    char *m_pszPostData;

} webapi_struct_t;


int ParseQueryString(webapi_struct_t *t, char *pszQueryString);
char *ConvertUriPath(const char *pszUriPath);
int reset_m_vpQuery(webapi_struct_t *t);

CHttpPostParam::CHttpPostParam()
: m_pszKey(NULL)
, m_pszValue(NULL)
{
}

CHttpPostParam::~CHttpPostParam()
{
    delete [] m_pszKey;
    delete [] m_pszValue;
}

static int get_dispatch_index(webapi_struct_t *t, const char *method, const char *url)
{
    int i;

    for(i=0; i<t->num_dispatch; ++i) 
    {
        if(0 == strcmp(url, t->dispatch[i].request) && 0 == strcmp(method, t->dispatch[i].method) &&
                NULL!=t->dispatch[i].callback)
        {
            return i;
        }
    }

    return -1;
}

static int queue_response_from_json_str(struct MHD_Connection *connection, char *str)
{
    struct MHD_Response *response;
    int ret;

    response = MHD_create_response_from_buffer(strlen(str), str, MHD_RESPMEM_MUST_FREE);
    if(NULL == response) {
        LOG("null response\n");
        LOG("-\n");
        return MHD_NO;
    }

    ret = MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json");
    if(MHD_NO == ret) {
        LOG("adding header failed\n");
        LOG("-\n");
        return ret;
    }

    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    if(MHD_NO == ret) {
        LOG("queueing response failed\n");
    }
    MHD_destroy_response(response);

    return MHD_YES;
}

// handle unknown URL with a 404 not found response
static int enqueue_unknown_response(struct MHD_Connection *connection)
{
    //static char *unknown_reply = "{result: -1}\n";
    static char unknown_reply[WEB_PARAM_LEN] = "{result: -1}\n";
    struct MHD_Response *response;
    int ret;

    response = MHD_create_response_from_buffer(strlen(unknown_reply), unknown_reply, MHD_RESPMEM_PERSISTENT);
    if(NULL == response) {
        LOG("null response\n");
        LOG("-\n");
        return MHD_NO;
    }

    ret = MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json");
    if(MHD_NO == ret) {
        LOG("adding header failed\n");
        LOG("-\n");
        return ret;
    }

    ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    if(MHD_NO == ret) {
        LOG("queueing response failed\n");
    }
    MHD_destroy_response(response);
    return MHD_YES;
}

static int handle_get_request(webapi_struct_t *t, struct MHD_Connection *connection,
                              int dispatch_index,
                              const char *method, const char *url)
{
    int ret;
    const char *str = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Content-Type");
    LOG("%s %s ctype=%s\n", method, url, str);
    
    //外部用
    webapi_data_t *v;
    v = (webapi_data_t *)calloc(1, sizeof(webapi_data_t));
    v->method = "GET";
    v->url = url;
    
    //內部用    //this will free here
    webapi_request_t *req;
    req = (webapi_request_t *)calloc(1, sizeof(webapi_request_t));
    req->connection = connection;
    req->dispatch_index = dispatch_index;
    req->connection = connection;
    req->method = "GET";
    t->webapi_request = req;

    if(str)
    {
        strcpy(v->content_type, str);
        strcpy(req->content_type, str);
    }

    char *reply = t->dispatch[dispatch_index].callback(t, v);
    
    if(v)
    {
        free(v);
        v = NULL;
    }
    
    if(req)
    {
        free(req);
        req = NULL;
    }
    
    LOG("handle_get_request \n");

    if(NULL == reply) 
    {
        LOG("callback returned NULL\n");
        return MHD_NO;
    }
    else 
    {
        ret = queue_response_from_json_str(connection, reply);
        return ret;
    }
}

static int handle_post_request(webapi_struct_t *t, struct MHD_Connection *connection,
                               int dispatch_index,
                               const char *method, const char *url,
                               const char *upload_data,
                               size_t *upload_data_size,
                               void **con_cls)
{
    int ret;
    webapi_request_t *req;

    if(NULL == *con_cls) 
    {
        //LOG("NULL == *con_cls\n");
        const char *str, *ctype;
        //LOG("new request %s %s\n", method, url);
        ctype = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Content-Type");
        LOG("%s %s ctype=%s\n", method, url, ctype);

        //內部用 (this will free in request_completed)
        req = (webapi_request_t *)calloc(1, sizeof(webapi_request_t));
        req->dispatch_index = dispatch_index;
        req->connection = connection;
        req->method = "POST";
        strncpy(req->content_type, ctype, sizeof(req->content_type));
        
        // initialize context for this request
        str = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Content-Length");
        if(str) 
        {
            req->expected_data_length = atoi(str);
            req->data_buffer = (char *)calloc(req->expected_data_length + 1, sizeof(char)); //+1 for null terminator
        }

        req->last_data = 0;
        *con_cls = (void *)req;
        
        LOG("%s %s expect data bytes=%d, con_cls=%p\n", method, url, req->expected_data_length, req);
        //LOG("-\n");
        return MHD_YES;
    }
	else
	{
        //LOG("NULL != *con_cls\n");
	}
    
    req = (webapi_request_t *)*con_cls;
    t->webapi_request = req;
    
    // save POST data
    if(0 != *upload_data_size &&
       (req->last_data + (int)*upload_data_size) <= req->expected_data_length) 
    {
        memcpy(req->data_buffer + req->last_data, upload_data, *upload_data_size);
        req->last_data += *upload_data_size;
        LOG("%s %s receiving %d bytes\n", method, url, (int)*upload_data_size);
    }
    else if(0 == *upload_data_size) 
    {
        LOG("%s %s received all data %d bytes. Dispatch.\n", method, url, req->last_data);

        //外部用
        webapi_data_t *v;
        v = (webapi_data_t *)calloc(1, sizeof(webapi_data_t));
        v->method = "POST";
        v->url = url;
        strncpy(v->content_type, req->content_type, sizeof(req->content_type) );

        if(req->data_buffer) 
        {
            v->content_length = req->expected_data_length;
            v->postdata = (char *)calloc(req->expected_data_length + 1, sizeof(char));
            memcpy(v->postdata, req->data_buffer, req->last_data);
            ParseQueryString(t, req->data_buffer);
        }
        
        //LOG("req->data_buffer = %s\n", req->data_buffer);
        //LOG("v->postdata = %s\n", v->postdata);
        char *reply = t->dispatch[dispatch_index].callback(t, v);
        
        //clear post data query string
        reset_m_vpQuery(t);

        if(v->postdata)
        {
            free(v->postdata);
            v->postdata = NULL;
        }
        if(v)
        {
            free(v);
            v = NULL;
        }

        if(NULL == reply)
		{
            LOG("callback returned NULL\n");
            LOG("handle_post_request end1\n");
            return MHD_NO;
        }
        ret = queue_response_from_json_str(connection, reply);
        LOG("handle_post_request end2\n");
        return ret;
    }
    else 
    {
        LOG("%s %s POST do nothig: %d, %d\n", method, url, (int)*upload_data_size, req->expected_data_length);
    }

    *upload_data_size = 0;
    LOG("handle_post_request end3\n");
    return MHD_YES;
}

//4th version
static int answer_to_request(void *cls,
                             struct MHD_Connection *connection,
                             const char *url,
                             const char *method,
                             const char *version,
                             const char *upload_data,
                             size_t *upload_data_size,
                             void **con_cls)
{
    webapi_struct_t *t = (webapi_struct_t *)cls;
    int dispatch_index;

    //LOG("+ con_cls=%p, %s %s\n", *con_cls, method, url);

    // check if this request can be handled
    dispatch_index = get_dispatch_index(t, method, url);
    if(-1 == dispatch_index) 
    {
        LOG("unknwon request %s\n", url);
        return enqueue_unknown_response(connection);
    }
    LOG("new request %s %s\n", method, url);

    // handle GET and POST requests separately
    if(0 == strcmp("GET", method)) 
    {
	    LOG("GET method\n");
        return handle_get_request(t, connection, dispatch_index, method, url);
    } 
    else if(0 == strcmp("POST", method)) 
    {
        LOG("POST method\n");
        return handle_post_request(t, connection, dispatch_index, method, url, upload_data, upload_data_size, con_cls);
    }
    else 
    {
        LOG("unknwon request %s %s\n", method, url);
        return enqueue_unknown_response(connection);
    }

    // shout not be here
    LOG("BUG. should not be here\n");
    return MHD_NO;
}

static void request_completed(void* cls, struct MHD_Connection* connection,
                       void** con_cls, enum MHD_RequestTerminationCode toe)
{
    webapi_request_t *req = (webapi_request_t *)*con_cls; 

    // do clean up
    if(req) 
    {
        LOG("+free request1\n");
        if(req->data_buffer) free(req->data_buffer);
        free(req);
        *con_cls = NULL;
        LOG("-free request1\n");
    }
    else
    {
        LOG("free request2\n");
    }
    LOG("\n\n");

}

WEBAPI_RESULT webapi_start(webapi_t *pt, int port, void *data, request_dispatch_t *dispatch, int num_dispatch)
{
    // allocate internal data structure
    webapi_struct_t *t = (webapi_struct_t *)malloc(sizeof(webapi_struct_t));
    if(NULL==t) return WEBAPI_ENOMEM;
    else *pt = t;

    // initialize context
    t->dispatch = dispatch;
    t->num_dispatch = num_dispatch;
    t->private_data = data;

    t->d = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY | MHD_USE_DEBUG,
                            port, 
                            NULL, NULL, 
                            answer_to_request, t,
                            MHD_OPTION_THREAD_POOL_SIZE, (unsigned int)1,
                            MHD_OPTION_NOTIFY_COMPLETED, request_completed, t,
                            MHD_OPTION_END);
    if(NULL == t->d) 
    {
        LOG("fail to start MHD daemon\n");
        free(t);
        return WEBAPI_PARAM_NULL_POINT;
    }

    return WEBAPI_SUCCESS;
}

WEBAPI_RESULT webapi_stop(webapi_t ot)
{
    webapi_struct_t *t = (webapi_struct_t *)ot;
    MHD_stop_daemon(t->d);
    free(t);
    return WEBAPI_SUCCESS;
}

int ParseQueryString(webapi_struct_t *t, char *pszQueryString)
{
    if(pszQueryString == NULL)
        return WEBAPI_PARAM_NULL_POINT;
    
    string strQueryString(pszQueryString);
    string::size_type nPos1 = 0, nPos2 = strQueryString.find("=");
    string strKey, strValue;
    //while(nPos2 != -1)
    while(nPos2 != string::npos)
    {
        strKey = strQueryString.substr(nPos1, nPos2 - nPos1);
        nPos1 = nPos2 + 1;
        nPos2 = strQueryString.find("&", nPos2);//10
        strValue= strQueryString.substr(nPos1, nPos2 - nPos1);
        CHttpPostParam *pParam = NULL;
        try
        {
            int nLen;
            pParam = new CHttpPostParam;
            char * pszKey = ConvertUriPath(strKey.c_str());
            nLen = strlen(pszKey);
            pParam->m_pszKey = new char[nLen + 1 + 100];
            strcpy(pParam->m_pszKey, pszKey);
            char * pszValue = ConvertUriPath(strValue.c_str());
            nLen = strlen(pszValue);
            pParam->m_pszValue = new char[(2 * nLen) + 1];
            strcpy(pParam->m_pszValue, pszValue);
            delete [] pszKey;
            delete [] pszValue;
            //t->webapi_request->m_vpQuery.push_back(pParam);
            m_vpQuery.push_back(pParam);

            if (string::npos == nPos2)
                break;
        }
        catch(...)
        {
            delete pParam;
            break;
        }
        nPos1 = nPos2 + 1;
        nPos2 = strQueryString.find("=", nPos2);
    }

    return 0;
}

char *ConvertUriPath(const char *pszUriPath)
{
    char *pszParsing = NULL;
    unsigned int unIndex, unUriIndex;
    unsigned int unPathLen;

    unPathLen = strlen(pszUriPath);
    try
    {
        pszParsing = new char[unPathLen + 1];
    }
    catch(...)
    {
        return NULL;
    }
    for(unIndex = 0, unUriIndex = 0; unIndex < unPathLen; unIndex++, unUriIndex++)
    {
        if (pszUriPath[unIndex] == '%')
        {
            if (pszUriPath[unIndex + 1] >= 'a' && pszUriPath[unIndex + 1] < 'g')
                pszParsing[unUriIndex] = (pszUriPath[unIndex + 1] - 'a' + 10) << 4;
            else if (pszUriPath[unIndex + 1] >= 'A' && pszUriPath[unIndex + 1] < 'G')
            	pszParsing[unUriIndex] = (pszUriPath[unIndex + 1] - 'A' + 10) << 4;
            else
                pszParsing[unUriIndex] = (pszUriPath[unIndex + 1] - '0') << 4;

            if (pszUriPath[unIndex + 2] >= 'a' && pszUriPath[unIndex + 2] < 'g')
                pszParsing[unUriIndex] += pszUriPath[unIndex + 2] - 'a' + 10;
            else if (pszUriPath[unIndex + 2] >= 'A' && pszUriPath[unIndex + 2] < 'G')
                pszParsing[unUriIndex] += pszUriPath[unIndex + 2] - 'A' + 10;
            else
                pszParsing[unUriIndex] += pszUriPath[unIndex + 2] - '0';

            unIndex += 2;
        }
        else if (pszUriPath[unIndex] == '+')
        {
            pszParsing[unUriIndex] = ' ';
        }
        else
        {
            pszParsing[unUriIndex] = pszUriPath[unIndex];
        }
    }
    pszParsing[unUriIndex] = '\0';

    return pszParsing;
}

WEBAPI_RESULT webapi_get_request_value(webapi_t t, const char *key, char *value, const unsigned int length)
{
    webapi_struct_t *t1;
    t1 = (webapi_struct_t *) t;
    MHD_Connection *connection = t1->webapi_request->connection;
    const char *method = t1->webapi_request->method;

    if( method == NULL || key == NULL)
        return WEBAPI_PARAM_NULL_POINT;
    
    //LOG("t1->webapi_request->method = %s\n", t1->webapi_request->method);

    WEBAPI_RESULT result = WEBAPI_NO_DEFINE;
    
    if(strcmp(method,"GET")== 0 )
    {
        const char *val;
        val = MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, key);
        if(val)
        {
            strncpy(value, val, length);
            result = WEBAPI_SUCCESS;
        }
        else
        {
            strcpy(value, "");
            result = WEBAPI_NO_VALUE;
        }
    }
    else if(strcmp(method,"POST")== 0 )
    {
        std::vector<CHttpPostParam *>::size_type unIndex, unNum;
        //unNum = t1->webapi_request->m_vpQuery.size();
        unNum = m_vpQuery.size();
        for(unIndex = 0; unIndex < unNum; unIndex++)
        {
            //if (strcmp(key, t1->webapi_request->m_vpQuery[unIndex]->m_pszKey) == 0)
            if (strcmp(key, m_vpQuery[unIndex]->m_pszKey) == 0)
            {
                //strncpy(value, t1->webapi_request->m_vpQuery[unIndex]->m_pszValue, length);
                strncpy(value, m_vpQuery[unIndex]->m_pszValue, length);
                result = WEBAPI_SUCCESS;
                break;
            }
            else
            {
                strcpy(value, "NULL");
                result = WEBAPI_NO_VALUE;
            }
        }
    }
    else
    {
        result = WEBAPI_NO_PARAM;
    }   
    return result;
}

int safeAtoi(const char *pPtr)
{
    int nRetval = -1;
    if (pPtr)
    {
        nRetval = atoi(pPtr);
    }

    return nRetval;
}

int reset_m_vpQuery(webapi_struct_t *t)
{
    std::vector<CHttpPostParam *>::size_type unIndex, unNum;
	
    //unNum = t->webapi_request->m_vpQuery.size();
    unNum = m_vpQuery.size();
    
    for(unIndex = 0; unIndex < unNum; unIndex++)
    {
        //delete t->webapi_request->m_vpQuery[unIndex];
        delete m_vpQuery[unIndex];
    }

    //t->webapi_request->m_vpQuery.clear();
    m_vpQuery.clear();

    return 0;
}

