/*
*   By: Rafael Tonello (tonello.rafinha@gmail.com)
*   Version: 0.0.0.0 (09/08/2017)
*/
#ifndef KWTINYWEBSERVER_H
#define KWTINYWEBSERVER_H

#include <pthread.h>
#include <sys/types.h>
#include <iterator>
#include <string.h>
#include <string>
#include <list>
#include <vector>
#include "../StringUtils.h"
#include "../SysLink.h"
#include <fcntl.h>
#include <iostream>
#include <stdio.h>

#ifdef __WIN32__
# include <winsock2.h>
#else
# include <sys/socket.h>
#endif

#include <netdb.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace Shared{
    using namespace std;

    class HttpData{
        public:
            string resource;
            string method;
            string contentType;
            char* contentBody = NULL;
            unsigned int contentLength = 0;
            vector< vector<string> > headers;
            unsigned int httpStatus;
            string httpErrorMessage;

            void setContentString(string data)
            {
                this->contentBody = new char[data.size()];
                for (int cont = 0; cont < data.size(); cont++)
                    this->contentBody[cont] = data[cont];
            }
    };

    //typedef void (*OnClientDataSend)(HttpData* in, HttpData* out);

    class WebServerObserver{
        public: virtual void OnClientDataSend(HttpData* in, HttpData* out) = 0;
    };

    class KWTinyWebServer
    {
        public:
            KWTinyWebServer(int port, WebServerObserver *observer, vector<string> filesLocations);
            virtual ~KWTinyWebServer();

            void __TryAutoLoadFiles(HttpData* in, HttpData* out);

            int __port;
            WebServerObserver *__observer;
            string __serverName;

        protected:

        private:
            vector<string> __filesLocations;
            pthread_t ThreadAwaitClients;
            SysLink sysLink;
    };

    void *ThreadAwaitClientsFunction(void *thisPointer);
    void *ThreadTalkWithClientFunction(void *arguments);
    bool SetSocketBlockingEnabled(int fd, bool blocking);
    void addStringToCharList(vector<char> *destination, string *source, char*source2, int source2Length = -1);
    bool __SocketIsConnected( int socket);
}
#endif // KWTINYWEBSERVER_H
