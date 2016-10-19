#ifndef __UTIL_H__
#define __UTIL_H__
#include "Common.h"
/*----------------------------错误码-----------------------------------------*/
#define ERR_CREATE_SOCKET   -1 
#define ERR_HOSTBYNAME      -2 
#define ERR_CONNECT         -3 
#define ERR_SEND            -4
#define ERR_TIMEOUT         -5
#define ERR_RECV            -6

typedef struct{
	osThreadId taskSend;
	osThreadId taskRec;
	void* client;	
}DeleteTaskList;


#ifdef __cpluscpluc
extern  "C" {
#endif
#include "cJSON.h"
//cJSON * makeUploadDataJson();

int32 Open(const char *addr, int16 portno);


#ifdef __cpluscpluc
}
#endif

#endif
