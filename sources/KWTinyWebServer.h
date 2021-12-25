

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

//#ifdef __WIN32__
//# include <winsock2.h>
//#else
//# include <sys/socket.h>
//#endif

//#include <netdb.h>
//#include <sys/ioctl.h>
#include <unistd.h>
#include <openssl/sha.h>
#include "ThreadPool.h"
#include<sys/time.h>

#include "SysLink.h"
#include "HttpData.h"
#include "StringUtils.h"
#include "IWorker.h"
#include "CookieParser.h"
#include <libgen.h>
#include <unistd.h>
#include <limits.h>

#include <TCPServer.h>

using namespace TCPServerLib;
namespace KWShared{
    using namespace std;

    //typedef void (*OnClientDataSend)(HttpData* in, HttpData* out);

    enum States {
        AWAIT_HTTP_FIRST_LINE, 
        READING_HEADER, 
        AWAIT_CONTENT, 
        SEND_REQUEST_TO_APP, 
        CHECK_PROTOCOL_CHANGE, 
        SEND_RESPONSE, 
        SEND_RESPONSE_WEBSOCKET,
        FINISH_REQUEST, 
        FINISHED, 
        ERROR_400_BADREQUEST, 
        ERROR_500_INTERNALSERVERERROR
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
            virtual void OnHttpRequest(HttpData* in, HttpData* out) = 0;
            virtual void OnWebSocketConnect(HttpData *originalRequest, string resource) = 0;
            virtual void OnWebSocketData(HttpData *originalRequest, string resource, char* data, unsigned long long dataSize) = 0;
            virtual void OnWebSocketDisconnect(HttpData *originalRequest, string resource) = 0;
    };

    class KWClientSessionState{
    public:
        string internalServerErrorMessage = "";
        HttpData receivedData, dataToSend;
        States state = AWAIT_HTTP_FIRST_LINE;
        States prevState = AWAIT_HTTP_FIRST_LINE;
        string connection = "";
        string upgrade = "";
        unsigned int currentContentLength = 0;
        bool webSocketOpen = false;
        bool webSocketState = WebSocketStates::WS_READING_PACK_INFO_1;
    };


    class KWTinyWebServer
    {
        public:
            KWTinyWebServer(
                int port,
                WebServerObserver *observer,
                vector<string> filesLocations,
                string dataFolder = "_AUTO_DEFINE_",
                ThreadPool* tasker = NULL
            );

            virtual ~KWTinyWebServer();

            void sendWebSocketData(ClientInfo *client, char* data, int size, bool isText);
            void sendWebSocketData(HttpData *originalRequest, char* data, int size, bool isText);

            void __TryAutoLoadFiles(HttpData* in, HttpData* out);
            WebServerObserver *__observer;
            string __serverName;
            string __dataFolder;
            ThreadPool * __tasks;

            void debug(string debug, bool forceFlush = false);
            long int getCurrDayMilisec();


            string getDataFolder(){ return this->__dataFolder; }
            StringUtils _strUtils;

            void addWorker(IWorker* worker);
            vector<IWorker*> __workers = {
                new CookieParser()
            };
        private:
            map<int, KWClientSessionState*> clientsSessionsStates;
            vector<string> __filesLocations;
            string get_app_path();
            SysLink sysLink;
            StringUtils strUtils;
            TCPServer* server;

            void initializeClient(ClientInfo* client);
            void finalizeClient(ClientInfo* client);
            void dataReceivedFrom(ClientInfo* client, char* data, size_t dataSize);
            void WebSocketProcess(ClientInfo* client, char* data, size_t dataSize);
    };

    //this class allow the instance of the webserver without implement the WebServerObserver interface.
    //TODO:Create a KWHelpers.h
    //TODO:Create an API helper
    class WebServerObserverHelper: public WebServerObserver
    {
    private:
        function<void(HttpData* in, HttpData* out)> onHttpRequest;
        function<void(HttpData *originalRequest, string resource)> onWebSocketConnect;
        function<void(HttpData *originalRequest, string resource, char* data, unsigned long long dataSize)> onWebSocketData;
        function<void(HttpData *originalRequest, string resource)> onWebSocketDisconnect;
    public:
        WebServerObserverHelper(
            function<void(HttpData* in, HttpData* out)> onHttpRequest,
            function<void(HttpData *originalRequest, string resource)> onWebSocketConnect = [](HttpData *originalRequest, string resource){},
            function<void(HttpData *originalRequest, string resource, char* data, unsigned long long dataSize)> onWebSocketData = [](HttpData *originalRequest, string resource, char* data, unsigned long long dataSize){},
            function<void(HttpData *originalRequest, string resource)> onWebSocketDisconnect = [](HttpData *originalRequest, string resource){}
        ){
            this->onHttpRequest = onHttpRequest;
            this->onWebSocketConnect = onWebSocketConnect;
            this->onWebSocketData = onWebSocketData;
            this->onWebSocketDisconnect = onWebSocketDisconnect;
        }

        int run(){
            while (true) usleep(10000);
        }

        void OnHttpRequest(HttpData* in, HttpData* out)
        {
            this->onHttpRequest(in, out);
        }

        void OnWebSocketConnect(HttpData *originalRequest, string resource)
        {
            this->onWebSocketConnect(originalRequest, resource);
        }

        void OnWebSocketData(HttpData *originalRequest, string resource, char* data, unsigned long long dataSize)
        {
            this->onWebSocketData(originalRequest, resource, data, dataSize);
        }

        void OnWebSocketDisconnect(HttpData *originalRequest, string resource)
        {
            this->onWebSocketDisconnect(originalRequest, resource);
        }
    };
}
#endif
