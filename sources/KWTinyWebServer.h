

/*
*   By: Rafael Tonello (tonello.rafinha@gmail.com)
*   Version: 0.0.0.0 (09/08/2017)
*/



/*

    Web socket requires libssl-dev package (in debian based install using "sudo apt install libssl-dev") and compile with -lssl and -lcrypto
*/
#ifndef KWTINYWEBSERVER_H
#define KWTINYWEBSERVER_H


#include <pthread.h>
#include <sys/types.h>
#include <iterator>
#include <string.h>
#include <string>
#include <list>
#include <fcntl.h>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <errno.h>
#include <queue>

//#ifdef __WIN32__
//# include <winsock2.h>
//#else
//# include <sys/socket.h>
//#endif

//#include <netdb.h>
//#include <sys/ioctl.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <ThreadPool.h>
#include <sys/time.h>

#include "SysLink.h"
#include "HttpData.h"
#include "StringUtilFuncs.h"
#include "IWorker.h"
#include "Workers/CookieParser.h"
#include <libgen.h>
#include <unistd.h>
#include <limits.h>
#include <ctime>

#include <TCPServer.h>

using namespace TCPServerLib;
namespace KWShared{
    using namespace std;

    //typedef void (*OnClientDataSend)(shared_ptr<HttpData> in, shared_ptr<HttpData> out);

    enum States {
        READING_VERB,
        AWAIT_REMAIN_OF_HTTP_FIRST_LINE, 
        READING_HEADER, 
        AWAIT_CONTENT, 
        SEND_REQUEST_TO_APP, 
        CHECK_PROTOCOL_CHANGE, 
        SEND_RESPONSE, 
        SEND_RESPONSE_WEBSOCKET,
        FINISH_REQUEST, 
        FINISHED, 
        ERROR_400_BADREQUEST, 
        ERROR_500_INTERNALSERVERERROR,
        ERROR_501_NOT_IMPLEMENTED
    };

    enum WebSocketStates{
        WS_READING_PACK_INFO_1,
        WS_READING_PACK_INFO_2,
        WS_READ_16BIT_SIZE,
        WS_READ_64BIT_SIZE,
        WS_READING_MASK_KEY,
        WS_READING_PAYLOAD,
        WS_FINISHED,
        WS_INTERNAL_SERVER_ERROR,
        WS_PAYLOAD_NOT_MASKED
    };

    class WebServerObserver{
        public:
            virtual void OnHttpRequest(shared_ptr<HttpData> in, shared_ptr<HttpData> out) = 0;
            virtual void OnWebSocketConnect(shared_ptr<HttpData>originalRequest, string resource) = 0;
            virtual void OnWebSocketData(shared_ptr<HttpData>originalRequest, string resource, char* data, unsigned long long dataSize) = 0;
            virtual void OnWebSocketDisconnect(shared_ptr<HttpData>originalRequest, string resource) = 0;
    };

    class KWClientSessionState{
    public:
        string internalServerErrorMessage = "";
        shared_ptr<ClientInfo>  client = nullptr;
        shared_ptr<HttpData> receivedData, dataToSend;
        States state = READING_VERB;
        States prevState = READING_VERB;
        string connection = "";
        string upgrade = "";
        unsigned int currentContentLength = 0;
        bool webSocketOpen = false;
        WebSocketStates webSocketState = WebSocketStates::WS_READING_PACK_INFO_1;


        string bufferStr = "";
        mutex incomingDataLocker;
        atomic<bool> processingIncomingData;
        queue<char> incomingDataBuffer = {};

        vector<string> tempHeaderParts;
        
        bool ignoreKeepAlive = false;

        int ws_tempIndex = 0;
        char ws_packSize7bit;
        int16_t ws_packSize16bit = 0;
        char ws_mask[4];
        bool ws_fin;
        char *ws_packPayload = nullptr;
        vector<char *> ws_payload;
        vector<int> ws_payloadSizes;
        unsigned long long ws_totalPayload = 0;
        unsigned long long ws_packSize = 0;
        bool ws_masked;
        char ws_opcode; //4 bits

        KWClientSessionState(){
            receivedData = make_shared<HttpData>();
            dataToSend = make_shared<HttpData>();
        }

        ~KWClientSessionState(){
            for (auto &c: ws_payload)
                delete[] c;
            ws_payload.clear();
        }
    };

    class KWTinyWebServer
    {
        public:
            KWTinyWebServer(
                int port,
                WebServerObserver *observer,
                vector<string> filesLocations,
                string dataFolder = "_AUTO_DEFINE_",
                ThreadPool* tasker = NULL,
                bool enableHttps = false,
                string sslKey = "",
                string sslPublicCert = ""
            );

            virtual ~KWTinyWebServer();

            void sendWebSocketData(shared_ptr<ClientInfo> client, char* data, int size, bool isText);
            void sendWebSocketData(shared_ptr<HttpData>originalRequest, char* data, int size, bool isText);
            void broadcastWebSocker(char* data, int size, bool isText, string resource = "*");
            void disconnecteWebSocket(shared_ptr<ClientInfo> client);
            void disconnecteWebSocket(shared_ptr<HttpData> originalRequest);


            void __TryAutoLoadFiles(shared_ptr<HttpData> in, shared_ptr<HttpData> out);
            WebServerObserver *__observer;
            string __dataFolder;
            ThreadPool * __tasks;

            void debug(string debug, bool forceFlush = false);
            long int getCurrDayMilisec();


            string getDataFolder(){ return this->__dataFolder; }

            void addWorker(IWorker* worker);
            vector<IWorker*> __workers = {
                new CookieParser()
            };

            //changes the server info. The server info is sen to the clients as the header 'Server'
            void setServerInfo(string newServerInfo){
                this->_serverInfo = newServerInfo;
            }

            //get the current server infor. Initially the server info is the concatenation of _serverName and _serverVersion
            string getServerInfo(){
                return this->_serverInfo;
            }

            //returns the server name (the string 'KWTinyWebServer embeded server')
            string getServerName(){
                return this->_serverName;
            }

            //returns the server version of KWtinyWebServer
            string getServerVersion(){
                return this->_serverVersion;
            }
        private:
            //server info is setted in the contructor as the concatenation of _serverName and _serverVersion. 
            //this can be changed be callling the method 'setServerInfo'
            string _serverInfo;
            
            const string _serverName = "KWTinyWebServer embeded server";
            const string _serverVersion = "2.2.3";
            //                       | | |
            //                       | | +------> Bugs fixes and compilation
            //                       | +--------> New features
            //
            mutex clientsSessionsStatesMutex;
            map<int, KWClientSessionState*> clientsSessionsStates;
            vector<string> __filesLocations;
            string get_app_path();
            SysLink sysLink;
            TCPServer* server;

            void initializeClient(shared_ptr<ClientInfo> client);
            void finalizeClient(shared_ptr<ClientInfo> client);
            void dataReceivedFrom(shared_ptr<ClientInfo> client);
            void WebSocketProcess(shared_ptr<ClientInfo> client);
        
            static bool isToKeepAlive(KWClientSessionState* sessionState);
            void endHttpRequest(shared_ptr<ClientInfo> client, KWClientSessionState* sessionState);
    };
}
#endif
