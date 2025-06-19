

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
        function<void(shared_ptr<HttpData> in, shared_ptr<HttpData> out)> onHttpRequest;
        function<void(shared_ptr<HttpData>originalRequest, string resource)> onWebSocketConnect;
        function<void(shared_ptr<HttpData>originalRequest, string resource, char* data, unsigned long long dataSize)> onWebSocketData;
        function<void(shared_ptr<HttpData>originalRequest, string resource)> onWebSocketDisconnect;
    public:
        WebServerObserverHelper(
            function<void(shared_ptr<HttpData> in, shared_ptr<HttpData> out)> onHttpRequest,
            function<void(shared_ptr<HttpData>originalRequest, string resource)> onWebSocketConnect = [](shared_ptr<HttpData>originalRequest, string resource){},
            function<void(shared_ptr<HttpData>originalRequest, string resource, char* data, unsigned long long dataSize)> onWebSocketData = [](shared_ptr<HttpData>originalRequest, string resource, char* data, unsigned long long dataSize){},
            function<void(shared_ptr<HttpData>originalRequest, string resource)> onWebSocketDisconnect = [](shared_ptr<HttpData>originalRequest, string resource){}
        ){
            this->onHttpRequest = onHttpRequest;
            this->onWebSocketConnect = onWebSocketConnect;
            this->onWebSocketData = onWebSocketData;
            this->onWebSocketDisconnect = onWebSocketDisconnect;
        }

        int run(){
            while (true) usleep(10000);
        }

        void OnHttpRequest(shared_ptr<HttpData> in, shared_ptr<HttpData> out)
        {
            this->onHttpRequest(in, out);
        }

        void OnWebSocketConnect(shared_ptr<HttpData>originalRequest, string resource)
        {
            this->onWebSocketConnect(originalRequest, resource);
        }

        void OnWebSocketData(shared_ptr<HttpData>originalRequest, string resource, char* data, unsigned long long dataSize)
        {
            this->onWebSocketData(originalRequest, resource, data, dataSize);
        }

        void OnWebSocketDisconnect(shared_ptr<HttpData>originalRequest, string resource)
        {
            this->onWebSocketDisconnect(originalRequest, resource);
        }
    };
}
#endif

