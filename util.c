#include "util.h" 

//cJSON * makeUploadDataJson(){
//    cJSON * datastreams = NULL;
//    cJSON * contentDatastreams = cJSON_CreateArray();   
//    cJSON * datapoints = cJSON_CreateObject();  
//    cJSON * datapointsArray = cJSON_CreateArray();  
//    char * pJson = cJSON_CreateObject();    
//    datastreams = cJSON_CreateObject();  
//    if(NULL == datastreams)
//    {
//        //error happend here
//        return NULL;
//    } 
//    extern sys_timer;    
//    cJSON_AddNumberToObject(pJson,"value",sys_timer);//数据点  
//    cJSON_AddItemToArray(datapointsArray,pJson);
//    cJSON_AddStringToObject(datapoints,"id","sys_time");
//    cJSON_AddItemToObject(datapoints,"datapoints",datapointsArray);
//    cJSON_AddItemToArray(contentDatastreams,datapoints);      
//    cJSON_AddItemToObject(datastreams,"datastradms",contentDatastreams);
//    
////    p = cJSON_PrintUnformatted(datastreams);
////    if(NULL == p){
////        cJSON_Delete(datastreams);
////
////        return NULL;
////    }
////    cJSON_Delete(datastreams);  
//    return datastreams;
//}

/* 
 * 函数名:  Open
 * 功能:    创建socket套接字并连接服务端
 * 参数:    addr    ip地址
 *          protno  端口号
 * 说明:    这里只是给出一个创建socket连接服务端的例子, 其他方式请查询相关socket api
 * 相关socket api:  
 *          socket, gethostbyname, connect
 * 返回值:  类型 (int32)
 *          <=0     创建socket失败
 *          >0      socket描述符
 */
//int32 Open(const char *addr, int16 portno)
//{
//    int32 sockfd;
//    struct sockaddr_in serv_addr;
//    struct hostent *server;
//
//    /* 创建socket套接字 */
//    sockfd = Socket(AF_INET, SOCK_STREAM, 0);
//    if (sockfd < 0) {
//        fprintf(stderr, "ERROR opening socket\n");
//        return ERR_CREATE_SOCKET; 
//    }
//    server = GetHostByName(addr);
//    if (server == NULL) {
//        fprintf(stderr, "ERROR, no such host\n");
//        return ERR_HOSTBYNAME;
//    }
//    bzero((char *) &serv_addr, sizeof(serv_addr));
//    serv_addr.sin_family = AF_INET;
//    bcopy((char *)server->h_addr, 
//          (char *)&serv_addr.sin_addr.s_addr,
//          server->h_length);
//    serv_addr.sin_port = htons(portno);
//    /* 客户端 建立与TCP服务器的连接 */
//    if (Connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
//    {
//        fprintf(stderr, "ERROR connecting\n");
//        return ERR_CONNECT;
//    }
//#ifdef _DEBUG
//    printf("[%s] connect to server %s:%d succ!...\n", __func__, addr, portno);
//#endif
//    return sockfd;
//}

