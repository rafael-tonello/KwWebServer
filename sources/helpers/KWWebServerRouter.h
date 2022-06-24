#ifndef __KWWEBSERVERROUTER__H__ 
#define __KWWEBSERVERROUTER__H__ 
#include "../KWTinyWebServer.h"
#include "WebServerObserverHelper.h"
#include "httpCodes.h"


 
namespace KWShared {

    class ServerRouterHttpException: public std::exception { 
    private:
        std::string _what = "Internal server error";
    public: 
        int httpCode = 500;
        string httpMessage = "Internal server error";
        ServerRouterHttpException(){}
        ServerRouterHttpException(int httpCode, string errorMessage, std::string httpMessage = "auto")
        {
            this->httpCode = httpCode;
            if (httpMessage == "auto")
                httpMessage = HttpCodes::get(httpCode);
            
            this->_what = errorMessage;

            this->httpMessage = httpMessage;
            
        };

        virtual const char * what() const noexcept{
            return (char*)this->_what.c_str();
        }
    }; 

    class KWWebServerRouter;
    namespace RoutesClasses
    {

        template <typename T>
        class KWSObservable{
        private:
            int currId = 0;
            mutex lock;
            map<int, T> observers;
        public:
            int add(T observer)
            {
                lock.lock(); 
                observers[currId++] = observer; 
                lock.unlock(); 
                return currId-1;
            }
            void remove(int id)
            { 
                if (observers.count(id))
                { 
                    lock.lock(); 
                    observers.erase(id); 
                    lock.unlock();
                } 
            }

            size_t size(){
                return observers.size();
            }
            
            map<int, T> getObservers(){ return observers;};
            void foreach(function<void(int id, T item)> f){
                for (auto &curr: observers)
                    f(curr.first, curr.second);
            }
        };

        class KWSRoute{
        public:
            string resource;
        };

        class KWSRequestRoute: public KWSRoute{
        protected:
            KWSObservable<function<void(HttpData* in, HttpData* out, map<string, string> vars)>> observers;
        public:
            string method;
            void onHttpRequest(function<void(HttpData* in, HttpData* out, map<string, string> vars)> f){ observers.add(f); };
        };

        class KWSRequestRouteExtended: public KWSRequestRoute{
        private:
            function<void (KWSRequestRouteExtended* route)> onDestroy = NULL;
        public:
            ~KWSRequestRouteExtended(){ if (onDestroy != NULL) onDestroy(this); }
            void setOnDestroy(function<void (KWSRequestRouteExtended* route)> f){ onDestroy = f; }
            KWSObservable<function<void(HttpData* in, HttpData* out, map<string, string> vars)>>& getObservers(){ return observers; };
        };

        class KWSWebsocketRoute: public KWSRoute{
        protected:
            KWWebServerRouter* router;

            KWSObservable<function<void(HttpData *originalRequest, map<string, string> vars)>> onConnectObservers;
            KWSObservable<function<void(HttpData *originalRequest, map<string, string> vars)>> onDisconnectObservers;
            KWSObservable<function<void(HttpData *originalRequest, map<string, string> vars, string data)>> onDataObservers;
        public:
            KWSWebsocketRoute(KWWebServerRouter* router):router(router){}

            void onConnect(function<void(HttpData *originalRequest, map<string, string> vars)> f){ onConnectObservers.add(f);};
            void onDisconnect(function<void(HttpData *originalRequest, map<string, string> vars)> f){ onDisconnectObservers.add(f);};
            void onData(function<void(HttpData *originalRequest, map<string, string> vars, string data)> f){ onDataObservers.add(f);};

            void send(HttpData *originalRequest, string data);
            void send(ClientInfo *client, string data);
            void sendToAll(string data);

            size_t observersCount();
        };

        class KWSWebsocketRouteExtended: public KWSWebsocketRoute{
        private:
            function<void (KWSWebsocketRouteExtended* route)> onDestroy = NULL;
        public:
            KWSWebsocketRouteExtended(KWWebServerRouter* router): KWSWebsocketRoute(router) {};
            void setOnDestroy(function<void (KWSWebsocketRouteExtended* route)> f){ onDestroy = f; }
            ~KWSWebsocketRouteExtended(){ if (onDestroy != NULL) onDestroy(this); }
            KWSObservable<function<void(HttpData *originalRequest, map<string, string> vars)>> &getOnConnectObservers(){ return onConnectObservers; };
            KWSObservable<function<void(HttpData *originalRequest, map<string, string> vars)>> &getOnDisconnectObservers(){ return onDisconnectObservers; };
            KWSObservable<function<void(HttpData *originalRequest, map<string, string> vars, string data)>> &getOnDataObservers(){ return onDataObservers; };
        };
    };

    using namespace RoutesClasses;

    class KWWebServerRouter{
    private:
        map<string, vector<KWSRoute*>> requestRoutesObservers;
        map<string, vector<KWSRoute*>> websocketRoutesObservers;
        vector<ClientInfo*> connectedWebsockets;

        shared_ptr<KWTinyWebServer> server;
        mutex listLockers;
        WebServerObserverHelper *serverObserver;
        StringUtils strUtils;
        
        void websocketRouteDestroyed(KWSWebsocketRouteExtended* route);
        void requestRouteDestroyed(KWSRequestRouteExtended* route);
        string fixRouteId(string route);
        void runLocked(function<void()> f);
        void OnHttpRequest(HttpData* in, HttpData* out);
        void OnWebSocketConnect(HttpData *originalRequest, string resource);
        void OnWebSocketData(HttpData *originalRequest, string resource, char* data, unsigned long long dataSize);
        void OnWebSocketDisconnect(HttpData *originalRequest, string resource);

        bool identifyObservers(string method, string resource, map<string, vector<KWSRoute*>> routes, function<void(KWSRoute* item, map<string, string> vars)> action);
        bool routeIsCompatible(string routeTemplate, string receivedResource, map<string, string> &vars);
        void extractUrlVars(string url, map<string, string>& vars);

        string getNestedExceptionText(exception &e, string prefix = "", int level = 0);
    public: 
        KWWebServerRouter(
            int port,
            vector<string> filesLocations,
            string dataFolder = "_AUTO_DEFINE_",
            ThreadPool* tasker = NULL
        ); 
        ~KWWebServerRouter();

        shared_ptr<KWSRequestRoute> route(string method, string resource);
        shared_ptr<KWSWebsocketRoute> routeWebsocket(string resource);
        shared_ptr<KWSRequestRoute> routeElse();
        shared_ptr<KWSWebsocketRoute> routeWebsocketElse();
        shared_ptr<KWTinyWebServer> getServer();
        vector<ClientInfo*> getConnectedWebsockets(){return connectedWebsockets;}
    };
} 
 
#endif 
