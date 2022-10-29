//compile with g++ -std=c++14 -o saida tester.cpp
#include <iostream>
#include <string>
#include <KWTinyWebServer.h>
#include <WebServerObserverHelper.h>
#include <HttpData.h>


using namespace std;
using namespace KWShared;

/*class Server: WebServerObserver
{
    public:
        Server(){
            int port = 5010;
            KWTinyWebServer *a = new KWShared::KWTinyWebServer(port, this, {});
            cout << "Server is waiting for connection on port " << port << endl;

        }

        int run(){
            while (true) usleep(10000);
        }

        void OnHttpRequest(HttpData* in, HttpData* out)
        {
            out->setContentString("Working!");

        }

        void OnWebSocketConnect(HttpData *originalRequest, string resource)
        {

        }

        void OnWebSocketData(HttpData *originalRequest, string resource, char* data, unsigned long long dataSize)
        {

        }

        void OnWebSocketDisconnect(HttpData *originalRequest, string resource)
        {

        }


};*/

int main()
{
    //Server server;
    //return server.run();
    KWTinyWebServer *a = new KWShared::KWTinyWebServer(12345, new WebServerObserverHelper(
        [](HttpData* in, HttpData* out){
            out->setContentString("Hey! The web page is working.");
        }
    ), {});

    system("xdg-open http://127.0.0.1:12345");


    while (true) usleep(10000);

}

