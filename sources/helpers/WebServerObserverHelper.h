

/*
*   By: Rafael Tonello (tonello.rafinha@gmail.com)
*   Version: 0.0.0.0 (09/08/2017)
*/



/*

    Web socket requires libssl-dev package (in debian based install using "sudo apt install libssl-dev") and compile with -lssl and -lcrypto
*/
#ifndef KWWERBSERVEROBSERVER_H
#define KWWERBSERVEROBSERVER_H

#include <KWTinyWebServer.h>

namespace KWShared{
    using namespace std;
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

