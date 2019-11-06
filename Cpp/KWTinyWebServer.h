
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

#ifdef __WIN32__
# include <winsock2.h>
#else
# include <sys/socket.h>
#endif

#include <netdb.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <openssl/sha.h>
#include "ThreadPool.h"
#include<sys/time.h>

#include "SysLink.h"
#include "HttpData.h"
#include "StringUtils.h"
#include "IWorker.h"
#include "CookieParser.h"
#include "HttpSession.h"


namespace KWShared{
    using namespace std;

    //typedef void (*OnClientDataSend)(HttpData* in, HttpData* out);

    class WebServerObserver{
        public:
            virtual void OnHttpRequest(HttpData* in, HttpData* out) = 0;
            virtual void OnWebSocketConnect(int client, string resource) = 0;
            virtual void OnWebSocketData(int client, string resource, char* data, unsigned long long dataSize) = 0;
            virtual void OnWebSocketDisconnect(int client, string resource) = 0;
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

            void sendWebSocketData(int client, char* data, int size, bool isText);

            void __TryAutoLoadFiles(HttpData* in, HttpData* out);
            int __port;
            WebServerObserver *__observer;
            string __serverName;
            ThreadPool * __tasks;

            void debug(string debug, bool forceFlush = false);
            long int
            getCurrDayMilisec();
            vector<IWorker*> workers = {
                new CookieParser()
            };

            string getDataFolder(){ return this->__dataFolder; }
        private:
            string __dataFolder;
            vector<string> __filesLocations;
            pthread_t ThreadAwaitClients;
            SysLink sysLink;
    };



    using namespace std;


    enum States {AWAIT_HTTP_FIRST_LINE, READING_HEADER, AWAIT_CONTENT, SEND_REQUEST_TO_APP, SEND_RESPONSE, FINISH_REQUEST, FINISHED, ERROR_400_BADREQUEST, ERROR_500_INTERNALSERVERERROR};


    static const std::string base64_chars ="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    typedef unsigned char BYTE;



}
#endif

