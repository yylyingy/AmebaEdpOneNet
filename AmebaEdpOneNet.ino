#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WiFiSSLClient.h>
#include <WiFiUdp.h>
#include "cJSON.h"
// #include "sockets.h"
//#include "netdb.h"
#include "Common.h"
#include "EdpKit.h"
#include "util.h"

#define _DEBUG	

// #define Socket(a,b,c)          socket(a,b,c)
// #define Connect(a,b,c)         connect(a,b,c)
// #define Close(a)               close(a)
// #define Read(a,b,c)            read(a,b,c)
// #define Recv(a,b,c,d)          recv(a, (void *)b, c, d)
// #define Select(a,b,c,d,e)      select(a,b,c,d,e)
// #define Send(a,b,c,d)          send(a, (const int8 *)b, c, d)
// #define Write(a,b,c)           write(a,b,c)
// #define GetSockopt(a,b,c,d,e)  getsockopt((int)a,(int)b,(int)c,(void *)d,(socklen_t *)e)
// #define SetSockopt(a,b,c,d,e)  setsockopt((int)a,(int)b,(int)c,(const void *)d,(int)e)
// #define GetHostByName(a)       gethostbyname((const char *)a)
int32 DoSend(WiFiClient client, const uint8* buffer, uint32 len);
int write_func(WiFiClient arg);

char ssid[] = "HONOR";      //  your network SSID (name)
char pass[] = "19920702";   // your network password
#define BAUDRATE 38400
// #define API_KEY "JJbbvahR5d4z6=0y7=dTo0espFI="
#define API_KEY "DhSSAeg0ufNI047x6alRwhahcnQ="//"JJbbvahR5d4z6=0y7=dTo0espFI=" //produce apikey
#define DEV_ID  "3507517"
#define SERVER_ADDR     "183.230.40.39"//"jjfaedp.hedevice.com"    //OneNet EDP 服务器地址
#define SERVER_PORT      876            //OneNet EDP 服务器端口

char *appID;
bool isNeedUploadToApp = false;
/**
 *每次发送完成时，判断sendTime与ackTime之间的时差，超过
 *ACK_TIME_OVERFLOW时调用WiFi.disconnect()(触发守护任务执行守护操作条件)
 *,每次判断完后给sendTime赋新值；收到ack时也要对actTime赋新值这样才能保证
 *正常联网时不调用WiFi.disconnect()函数，断网时则调用
*/
#define NONE_DST_DEVICE    ""
#define ACK_TIME_OVERFLOW   5000
long sendTime;											
long ackTime;
int sys_timer;

//uart out mutex
osMutexDef (uart_mutex);
osMutexId  (uart_mutex_id); // Mutex ID
void initJson(){
  cJSON_Hooks memoryHook;
  memoryHook.malloc_fn = pvPortMalloc;
  memoryHook.free_fn = vPortFree;
  cJSON_InitHooks(&memoryHook);
}


//WiFiClient client;
//char server[] = "183.230.40.39";
int status = WL_IDLE_STATUS;
/*
 * buffer按十六进制输出
 */
void hexdump(const unsigned char *buf, uint32 num)
{
	osMutexWait(uart_mutex_id, osWaitForever); 
    uint32 i = 0;
    for (; i < num; i++) 
    {
        printf("%02X ", buf[i]);
        if ((i+1)%8 == 0) 
            printf("\n");
    }
    printf("\n");
	osMutexRelease(uart_mutex_id);
}

int ledOn = LOW;
void messageHandler(cJSON* jsonRoot){
	long time = micros();
    cJSON * pJson ;//= cJSON_CreateObject();
	cJSON * datastreams = jsonRoot;
    cJSON * contentDatastreams ;//= cJSON_CreateArray();   
    cJSON * datapoints ;//= cJSON_CreateObject();  
    cJSON * datapointsArray = cJSON_CreateArray();
	contentDatastreams = cJSON_GetObjectItem(datastreams,"datastreams");
	//printf("\ncontentDatastreams:%s\n",cJSON_PrintUnformatted(contentDatastreams));
	
	datapoints = cJSON_GetArrayItem(contentDatastreams,0);
	//printf("\ndatapoints:%s\n",cJSON_PrintUnformatted(datapoints));
	cJSON* temp	= cJSON_GetObjectItem(datapoints,"id");	
	if(strcmp("LEDCTRL",temp->valuestring) != 0){
		//printf("\nid - LEDCTRL:%d\n",strcmp("LEDCTRL",temp->valuestring));
		return;
	}
	pJson = cJSON_GetObjectItem(datapoints,"datapoints");
	//printf("\npJson:%s\n",cJSON_PrintUnformatted(pJson));	
	cJSON * sJSON = cJSON_GetArrayItem(pJson,0);
	//printf("\nsJSON:%s\n",cJSON_PrintUnformatted(sJSON));
	temp	= cJSON_GetObjectItem(sJSON,"value");
	if(strcmp("LEDCTRL",temp->valuestring) == 0){        
        digitalWrite(11,ledOn);       
        ledOn = !ledOn;
		
		printf("\nchange led state!%d\n",ledOn);
	}
	printf("\nwaste time:%d\n",micros() - time);
}

static char buffer[512];
int recv_func(WiFiClient client)
{
    int error = 0;
    int n, rtn;
    uint8 mtype, jsonorbin;

    RecvBuffer* recv_buf = NewBuffer();
    EdpPacket* pkg;

    char* src_devid;
    char* push_data;
    uint32 push_datalen;

    cJSON* save_json;
    char* save_json_str;

    cJSON* desc_json;
    char* desc_json_str;
    char* save_bin;
    uint32 save_binlen;
    int ret = 0;
    do
    {
        //n = Recv(sockfd, buffer, 512, 0);
		memset(buffer, 0, sizeof(buffer));
		n = client.read((uint8_t*)(&buffer[0]), sizeof(buffer));
        if(n < 0)
        {
			osMutexWait(uart_mutex_id, osWaitForever); 
            printf("recv error, bytes: %d\n", n);
			osMutexRelease(uart_mutex_id);
            error = -1;
//			if(client.available()){
//				client.stop();
//			}			
			WiFi.disconnect();//触发守护任务条件
            break;
        }

		osMutexWait(uart_mutex_id, osWaitForever); 
        printf("recv from server, bytes: %d\n", n);
		osMutexRelease(uart_mutex_id);

        WriteBytes(recv_buf, buffer, n);
        while(1)
        {
            if((pkg = GetEdpPacket(recv_buf)) == 0)
            {
				osMutexWait(uart_mutex_id, osWaitForever); 
                printf("need more bytes...\n");
				osMutexRelease(uart_mutex_id);
                break;
            }

            mtype = EdpPacketType(pkg);
            switch(mtype)
            {
                case CONNRESP:
                    rtn = UnpackConnectResp(pkg);
					osMutexWait(uart_mutex_id, osWaitForever); 
                    printf("recv connect resp, rtn: %d\n", rtn);
					osMutexRelease(uart_mutex_id);
                    break;
				case SAVEACK:
                    ackTime = millis();
					vTaskSuspendAll();
					printf("\nrec ack for savedata function!\n");
					xTaskResumeAll();					
					break;
                case PUSHDATA:
                    UnpackPushdata(pkg, &src_devid, &push_data, &push_datalen);
					osMutexWait(uart_mutex_id, osWaitForever); 
                    printf("\nrecv push data, src_devid: %s, push_data: %s, len: %d\n",
                           src_devid, push_data, push_datalen);
					osMutexRelease(uart_mutex_id);
                    free(src_devid);
                    free(push_data);
                    break;
                case SAVEDATA:
                    if(UnpackSavedata(pkg, &src_devid, &jsonorbin) == 0)
                    {
                        if(jsonorbin == 0x01)
                        {
                            /* json */
                            ret = UnpackSavedataJson(pkg, &save_json);
                            save_json_str=cJSON_PrintUnformatted(save_json);
							osMutexWait(uart_mutex_id, osWaitForever); 
                            printf("recv save data json, ret = %d, src_devid: %s, json: %s\n",
                                   ret, src_devid, save_json_str);
						    osMutexRelease(uart_mutex_id);
                            appID = (char *)malloc(strlen(src_devid));
                            strcpy(appID,src_devid);
                            isNeedUploadToApp = true;
                            free(save_json_str);
							messageHandler(save_json);
                            cJSON_Delete(save_json);
                        }
                        else if(jsonorbin == 0x02)
                        {
                            /* bin */
                            UnpackSavedataBin(pkg, &desc_json, (uint8**)&save_bin, &save_binlen);
                            desc_json_str=cJSON_PrintUnformatted(desc_json);
							osMutexWait(uart_mutex_id, osWaitForever); 
                            printf("recv save data bin, src_devid: %s, desc json: %s, bin: %s, binlen: %d\n",
                                   src_devid, desc_json_str, save_bin, save_binlen);
						    osMutexRelease(uart_mutex_id);
                            free(desc_json_str);
                            cJSON_Delete(desc_json);
                            free(save_bin);
                        }
                        free(src_devid);
                    }
                    break;
                case PINGRESP:
                    UnpackPingResp(pkg);
					osMutexWait(uart_mutex_id, osWaitForever); 
                    printf("recv ping resp\n");
					osMutexRelease(uart_mutex_id);
					ackTime = millis();
                    break;
                default:
					osMutexWait(uart_mutex_id, osWaitForever); 
                    printf("recv failed...\n");
					osMutexRelease(uart_mutex_id);
                    break;
            }
            DeleteBuffer(&pkg);
        }
    }
    while(0);

    DeleteBuffer(&recv_buf);

    return error;
}
/* 
 * 函数名:  recv_thread_func
 * 功能:    接收线程函数
 * 参数:    arg     socket描述符
 * 说明:    这里只是给出了一个从socket接收数据的例子, 其他方式请查询相关socket api
 *          一般来说, 接收都需要循环接收, 是因为需要接收的字节数 > socket的读缓存区时, 一次recv是接收不完的.
 * 相关socket api:  
 *          recv
 * 返回值:  无
 */
void recv_thread_func(void const *arg){	
	vTaskSuspendAll();
	void * temp = const_cast<void *> (arg);
	//int sockfd = *(int*)arg;
	WiFiClient * temp2 = static_cast<WiFiClient *>(temp);
	WiFiClient client = *temp2;
	xTaskResumeAll();
	ackTime = millis();
	while(true){
	   recv_func(client);
	}
}

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
// int32 Open(const char *addr, int16 portno)
// {
    // int32 sockfd;
    // struct sockaddr_in serv_addr;
    // struct hostent *server;

    // /* 创建socket套接字 */
    // sockfd = Socket(AF_INET, SOCK_STREAM, 0);
    // if (sockfd < 0) {
        // fprintf(stderr, "ERROR opening socket\n");
        // return ERR_CREATE_SOCKET; 
    // }
    // server = GetHostByName(addr);
    // if (server == NULL) {
        // fprintf(stderr, "ERROR, no such host\n");
        // return ERR_HOSTBYNAME;
    // }
    // bzero((char *) &serv_addr, sizeof(serv_addr));
    // serv_addr.sin_family = AF_INET;
    // bcopy((char *)server->h_addr, 
          // (char *)&serv_addr.sin_addr.s_addr,
          // server->h_length);
    // serv_addr.sin_port = htons(portno);
    // /* 客户端 建立与TCP服务器的连接 */
    // if (Connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
    // {
        // fprintf(stderr, "ERROR connecting\n");
        // return ERR_CONNECT;
    // }
// #ifdef _DEBUG
	// osMutexWait(uart_mutex_id, osWaitForever); 
    // printf("[%s] connect to server %s:%d succ!...\n", __func__, addr, portno);
	// osMutexRelease(uart_mutex_id);
// #endif	
    // return sockfd;
// }

/* 
 * 函数名:  DoSend
 * 功能:    将buffer中的len字节内容写入(发送)socket描述符sockfd, 成功时返回写的(发送的)字节数.
 * 参数:    sockfd  socket描述符 
 *          buffer  需发送的字节
 *          len     需发送的长度
 * 说明:    这里只是给出了一个发送数据的例子, 其他方式请查询相关socket api
 *          一般来说, 发送都需要循环发送, 是因为需要发送的字节数 > socket的写缓存区时, 一次send是发送不完的.
 * 相关socket api:  
 *          send
 * 返回值:  类型 (int32)
 *          <=0     发送失败
 *          >0      成功发送的字节数
 */
int32 DoSend(WiFiClient client, const uint8* buffer, uint32 len)
{
    int32 total  = 0;
    int32 n = 0;
    while (len != total)
    {
        /* 试着发送len - total个字节的数据 */
        //n = Send(sockfd,buffer + total,len - total,0);
		n = client.write(buffer,len);
        if (n <= 0)
        {
            fprintf(stderr, "ERROR writing to socket\n");
            return n;
        }
        /* 成功发送了n个字节的数据 */
        total += n;
    }
    /* wululu test print send bytes */
#ifdef _DEBUG
    hexdump((const unsigned char *)buffer, len);
#endif
    return total;
}

int write_func(WiFiClient arg){
    unsigned int now = millis();
    WiFiClient client = arg;
    EdpPacket* send_pkg;
    cJSON * save_json;
    int32 ret = 0;
    char text[25] = {0};
    if(isNeedUploadToApp){
        sys_timer = now - 5001;//直接跳过等待两秒时间，当收到app控制信息时，需要马上上报数据
    }
    /*push data every 5 seconds*/
    if(now - sys_timer < 5000){
        return 0;
    }
    sys_timer = now;    
    /*产生json，其中携带了要上传的用户数据*/
    save_json = makeUploadDataJson();
    /*将json封装成edp数据包*/
    if(NULL == save_json){
		osMutexWait(uart_mutex_id, osWaitForever); 
        printf("invalid json!");
		osMutexRelease(uart_mutex_id);
        return -1;
    }
    char * p = cJSON_PrintUnformatted(save_json);
    printf("%s",p);
    free(p);
    
    //send_pkg = PacketSavedataJson(DEV_ID,save_json,1,0);
    if(isNeedUploadToApp){
        send_pkg = PacketSavedataJson(appID,save_json,1,1);
        isNeedUploadToApp = false;
        free(appID);
    }else{
	    send_pkg = PacketSavedataJson(NONE_DST_DEVICE,save_json,1,1);//0);
    }
    if(NULL == send_pkg){
        return -1;
    }    
    /*发送edp数据包上传数据*/
    ret = DoSend(client,send_pkg->_data,send_pkg->_write_pos);
	if(sendTime - ackTime > ACK_TIME_OVERFLOW){
//		if(client.available()){
//			client.stop();
//		}	
		WiFi.disconnect();
		printf("\nwifi force to disconnect\n");
	}
	sendTime = millis();
    DeleteBuffer(&send_pkg);
    cJSON_Delete(save_json);
    return ret; 
}
void edp_demo(void const *arg){
    int32 ret;//,sockfd;
	WiFiClient client;
	int sendHeartDataTimer = millis();;
    EdpPacket * send_pkg;
	DeleteTaskList *sockTaskList;
	void* taskTemp = const_cast<void*>(arg);
	sockTaskList = static_cast<DeleteTaskList *>(taskTemp);
    sys_timer = millis();
    //1.创建一个与服务器通讯的socket连接
    //sockfd = Open(SERVER_ADDR,SERVER_PORT);
	if (!client.connect(SERVER_ADDR, SERVER_PORT)) {
		WiFi.disconnect();//触发守护任务条件
		while(true);
		//vTaskDelete(NULL);
	}	
    //2.产生并发送edp设备连接请求的数据
    send_pkg = PacketConnect1(DEV_ID,API_KEY);
    ret = DoSend(client,send_pkg->_data,send_pkg->_write_pos);//登录
    DeleteBuffer(&send_pkg);
	osMutexWait(uart_mutex_id, osWaitForever); 
	printf("\nfirst ret = \n%d",ret);	
	osMutexRelease(uart_mutex_id);
	if(ret < 0){
//		if(client.available()){
//			client.stop();
//		}	
		osMutexWait(uart_mutex_id, osWaitForever); 
		printf("\nThe first send cause task delete!\n");
		osMutexRelease(uart_mutex_id);
		WiFi.disconnect();
		while(true);
		//vTaskDelete(NULL);
	}
	client.setRecvTimeout(0);//设置接收超时时间为永不超时，否则超时将会自动退出接收代码
	void *socket = static_cast<void *>(&client); 
	sockTaskList->client = socket;
	osThreadId idRecTask;
	uint32 rec_id = os_thread_create(recv_thread_func, socket, OS_PRIORITY_NORMAL, 4096);
	// void* temp = (unsigned int *)(&edp_demo_id);
	// idSendTask = *(static_cast<osThreadId *>(temp));
	// sockTaskList.taskSend = idSendTask;
	sendTime = millis();
	void* temp = (uint32 *)(&rec_id);
	idRecTask = *(static_cast<osThreadId *>(temp));

	sockTaskList->taskRec = idRecTask;	
	digitalWrite(12,HIGH);
    while(true){        
		if(millis() - sendHeartDataTimer > 100000){//心跳 num / 1000 s
			sendHeartDataTimer = millis();
			send_pkg = PacketPing();
			ret = DoSend(client,send_pkg->_data,send_pkg->_write_pos);
			DeleteBuffer(&send_pkg);
			printf("\npacketPing!\n");
		}
        ret = write_func(client);		
        if(ret < 0){
//			if(client.available()){
//				client.stop();
//			}	
#ifdef _DEBUG
			osMutexWait(uart_mutex_id, osWaitForever); 
            printf("\ntask delete!\n");
			osMutexRelease(uart_mutex_id);
#endif
			WiFi.disconnect();
            //vTaskDelete(NULL);
        }
        if(ret){
#ifdef _DEBUG	
			osMutexWait(uart_mutex_id, osWaitForever); 
            printf("\nupload data success!ret:%d\n",ret);		
			osMutexRelease(uart_mutex_id);
#endif	
        }
    }    
}

void appTaskStart( void const *arg ){ //守护任务，管理sendTask ， recTask  and socket
#ifdef _DEBUG
	osMutexWait(uart_mutex_id, osWaitForever);
    printf("appTaskStart task start successful!\n");       
	osMutexRelease(uart_mutex_id);     
#endif	
    unsigned int count = 0;
	volatile bool hasDeleteTask = false;	
	DeleteTaskList sockTaskList;
    uint32 edp_demo_id = os_thread_create(edp_demo, static_cast<void*>(&sockTaskList), OS_PRIORITY_NORMAL, 4096);   
	osThreadId idSendTask ;
	void* temp = (unsigned int *)(&edp_demo_id);
	idSendTask = *(static_cast<osThreadId *>(temp));
	sockTaskList.taskSend = idSendTask;
	sockTaskList.taskRec  = NULL;
	sockTaskList.client	= NULL;
    for(;;)  {  
        delay(2000);		
		if(WiFi.status() != WL_CONNECTED){//when the station disconnected from the ap，try to reconnect and delete tasks
			if(!hasDeleteTask){
				if(sockTaskList.taskSend != NULL){
					vTaskDelete(sockTaskList.taskSend);
					printf("\ntaskSend has delete!\n");
					sockTaskList.taskSend = NULL;
				}
				if(sockTaskList.taskRec != NULL){
					printf("\ntaskRec is not null,now delete it\n");					
					vTaskDelete(sockTaskList.taskRec);
					printf("\ndelete success!\n");
					sockTaskList.taskRec = NULL;
				}else{
					printf("\ntaskRec is null\n");
				}
				if(sockTaskList.client != NULL){
					if((*static_cast<WiFiClient*>(sockTaskList.client)).available()){
						(*static_cast<WiFiClient*>(sockTaskList.client)).stop();
					}
					sockTaskList.client = NULL;
				}

				digitalWrite(12,LOW);
				digitalWrite(13,LOW);
                hasDeleteTask = true;
				
			}			
			Serial.print("Attempting to connect to Network named: ");
			Serial.println(ssid);                   // print the network name (SSID);
			// Connect to WPA/WPA2 network. Change this line if using open or WEP network:
			status = WiFi.begin(ssid, pass);
			delay(1000);
		}		   
		if(WiFi.status() == WL_CONNECTED){
			if(hasDeleteTask){
				digitalWrite(13,HIGH);
				edp_demo_id = os_thread_create(edp_demo, static_cast<void*>(&sockTaskList), OS_PRIORITY_NORMAL, 4096);
				void* temp = (unsigned long *)(&edp_demo_id);
				idSendTask = *(static_cast<osThreadId *>(temp));
				sockTaskList.taskSend = idSendTask;
				hasDeleteTask = false;
			}						
		}
    }
}
void setup() {	
    initJson();
    // put your setup code here, to run once:
    Serial.begin(BAUDRATE);  
    // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    delay(1000);
  }
    pinMode(13,OUTPUT);     
	pinMode(12,OUTPUT);
    pinMode(11,OUTPUT);

	digitalWrite(13,HIGH);
	uart_mutex_id = osMutexCreate(osMutex(uart_mutex));  
    os_thread_create(appTaskStart, NULL, OS_PRIORITY_NORMAL, 2048); 	
	osThreadSetPriority(NULL,osPriorityNormal);
}

void loop(){
    delay(10000);    
}



