#include  "KWWebServerRouter.h" 

using namespace KWShared;
using namespace KWShared::RoutesClasses;



KWWebServerRouter::KWWebServerRouter(int port, vector<string> filesLocations, string dataFolder, ThreadPool* tasker) 
{ 
    serverObserver = new WebServerObserverHelper(
        [&](shared_ptr<HttpData> in, shared_ptr<HttpData> out){this->OnHttpRequest(in, out); },
        [&](shared_ptr<HttpData>originalRequest, string resource){this->OnWebSocketConnect(originalRequest, resource); },
        [&](shared_ptr<HttpData>originalRequest, string resource, char* data, unsigned long long dataSize){this->OnWebSocketData(originalRequest, resource, data, dataSize); },
        [&](shared_ptr<HttpData>originalRequest, string resource){this->OnWebSocketDisconnect(originalRequest, resource); }
    );

    server = shared_ptr<KWTinyWebServer>(new KWTinyWebServer(port, serverObserver, filesLocations, dataFolder, tasker));
} 
 
KWWebServerRouter::~KWWebServerRouter() 
{
    delete serverObserver;
}
 
shared_ptr<KWSWebsocketRoute> KWWebServerRouter::routeWebsocket(string resource)
{
    auto result = new KWSWebsocketRouteExtended(this);
    result->resource = resource;
    string routeId = resource != "*" ? fixRouteId("WS"+resource) : "**";

            
    runLocked([&](){
        if (!this->websocketRoutesObservers.count(routeId))
            this->websocketRoutesObservers[routeId] = {};
        this->websocketRoutesObservers[routeId].push_back(result);
    });

    result->setOnDestroy([&](KWSWebsocketRouteExtended* route){ 
        websocketRouteDestroyed(route); 
    });

    return shared_ptr<KWSWebsocketRoute>(result);
}

shared_ptr<KWSWebsocketRoute> KWWebServerRouter::routeWebsocketElse()
{
    return routeWebsocket("*");
}

shared_ptr<KWSRequestRoute> KWWebServerRouter::route(string method, string resource)
{
    auto result = new KWSRequestRouteExtended();
    result->method = method;
    result->resource = resource;
    //string routeId = fixRouteId(method+resource);
    string routeId = resource != "*" ? fixRouteId("WS"+resource) : "**";
            
    runLocked([&](){
        if (!this->requestRoutesObservers.count(routeId))
            this->requestRoutesObservers[routeId] = {};
        this->requestRoutesObservers[routeId].push_back(result);
    });

    result->setOnDestroy([&](KWSRequestRouteExtended* route){ 
        requestRouteDestroyed(route); 
    });
    return shared_ptr<KWSRequestRoute>(result);
}

shared_ptr<KWSRequestRoute> KWWebServerRouter::routeElse()
{
    return route("*", "*");
}

void KWWebServerRouter::websocketRouteDestroyed(KWSWebsocketRouteExtended* route)
{
    auto resource = fixRouteId(route->resource);
    if (websocketRoutesObservers.count(resource))
    {
        
        auto itemPos = std::find(websocketRoutesObservers[resource].begin(), websocketRoutesObservers[resource].end(), route);
        if (itemPos != websocketRoutesObservers[resource].end())
            runLocked([&](){
                this->websocketRoutesObservers[resource].erase(itemPos);
            });
        else
            cerr << "Try do deleting a not existing KWSWebsocketRouteExtended" << endl;
    }
}

void KWWebServerRouter::requestRouteDestroyed(KWSRequestRouteExtended* route)
{
    auto resource = fixRouteId(route->method+route->resource);
    if (requestRoutesObservers.count(resource))
    {
        
        auto itemPos = std::find(requestRoutesObservers[resource].begin(), requestRoutesObservers[resource].end(), route);
        if (itemPos != requestRoutesObservers[resource].end())
            runLocked([&](){
                this->requestRoutesObservers[resource].erase(itemPos);
            });
        else
            cerr << "Try do deleting a not existing KWSRequestRoute" << endl;
    }
}

shared_ptr<KWTinyWebServer> KWWebServerRouter::getServer() 
{
    return server;
}

void KWWebServerRouter::OnHttpRequest(shared_ptr<HttpData> in, shared_ptr<HttpData> out)
{

    try{
        bool routeFound = identifyObservers(in->method, in->resource, requestRoutesObservers, [&](KWSRoute* item, map<string, string> vars){
            ((KWSRequestRouteExtended*)item)->getObservers().foreach([&](int id, function<void(shared_ptr<HttpData> in, shared_ptr<HttpData> out, map<string, string>)> curr){
                curr(in, out, vars);
            });
        });

        //check 
        if (routeFound)
        {
            //chekc if httpstatus was setted or not by the a route handler
            if (out->httpStatus == 0)
            {
                out->httpStatus = 200;
                out->httpMessage = "Ok";
            }
        }
    }
    catch (ServerRouterHttpException &e)
    {
        out->httpStatus = e.httpCode;
        out->httpMessage = e.httpMessage;
        //out->setContentString(e.what());
        cerr << "KWWebServerRouter http exception, " << e.httpCode <<" - "<< e.httpMessage <<": "<< endl << getNestedExceptionText(e, "    ");;
        out->setContentString(getNestedExceptionText(e));
    }
    catch(std::exception &e)
    {
        out->httpStatus = 500;
        out->httpMessage = "Internal server error";
        //out->setContentString(e.what());
        cerr << "KWWebServerRouter exception, 500 - Internal server error: "<< endl << getNestedExceptionText(e, "    ");;
        out->setContentString(getNestedExceptionText(e));
    }
    catch(...)
    {
        out->httpStatus = 500;
        out->httpMessage = "Internal server error";
        cerr << "KWWebServerRouter exception, 500 - Internal server error: \n    Unknown error";
        out->setContentString("Unknown error");
    }
}

string KWWebServerRouter::getNestedExceptionText(exception &e, string prefix, int level)
{

    string ret;
    for (int c = 0; c < level; c++)
        ret += " ";
    //ret += "-> ";
    ret += prefix + e.what();

    try{
        rethrow_if_nested(e);
    }
    catch(exception &e2)
    {
        ret += "\n" + getNestedExceptionText(e2, prefix, level+1);
    }

    return ret;
}

void KWWebServerRouter::OnWebSocketConnect(shared_ptr<HttpData>originalRequest, string resource)
{
    connectedWebsockets.push_back(originalRequest->client);
    
    try{
        auto found = identifyObservers("WS", originalRequest->resource, websocketRoutesObservers, [&](KWSRoute* item, map<string, string> vars){
            ((KWSWebsocketRouteExtended*)item)->getOnConnectObservers().foreach([&](int id, function<void(shared_ptr<HttpData>oriReq, map<string, string>varList)> curr){
                curr(originalRequest, vars);
            });
        });

        if (!found)
        {
            originalRequest->client->disconnect();
        }
    }
    catch (ServerRouterHttpException &e)
    {
        originalRequest->client->disconnect();
        originalRequest->httpStatus = e.httpCode;
        originalRequest->httpMessage = e.httpMessage;
        //out->setContentString(e.what());
        cerr << "KWWebServerRouter http exception, " << e.httpCode <<" - "<< e.httpMessage <<": "<< endl << getNestedExceptionText(e, "    ");;
        originalRequest->setContentString(getNestedExceptionText(e));
    }
    catch(std::exception &e)
    {
        originalRequest->client->disconnect();
        originalRequest->httpStatus = 500;
        originalRequest->httpMessage = "Internal server error";
        //out->setContentString(e.what());
        cerr << "KWWebServerRouter exception, 500 - Internal server error: "<< endl << getNestedExceptionText(e, "    ");;
        originalRequest->setContentString(getNestedExceptionText(e));
    }
    catch(...)
    {
        originalRequest->client->disconnect();
        originalRequest->httpStatus = 500;
        originalRequest->httpMessage = "Internal server error";
        cerr << "KWWebServerRouter exception, 500 - Internal server error: \n    Unknown error";
        originalRequest->setContentString("Unknown error");
    }
}

void KWWebServerRouter::OnWebSocketData(shared_ptr<HttpData>originalRequest, string resource, char* data, unsigned long long dataSize)
{
    try
    {
        identifyObservers("WS", originalRequest->resource, websocketRoutesObservers, [&](KWSRoute* item, map<string, string> vars){
            ((KWSWebsocketRouteExtended*)item)->getOnDataObservers().foreach([&](int id, function<void(shared_ptr<HttpData>oriRq, map<string, string>varList, string dt)> curr){
                curr(originalRequest, vars, string(data, dataSize));
            });
        });
    }
    catch(std::exception &e)
    {
        string dt = "Internal server error: " + string(e.what());
        this->server->sendWebSocketData(originalRequest->client, (char*)dt.c_str(), (int)dt.size(), true);
    }
    catch(...)
    {
        string dt = "Internal server error. Unknown error.";
        this->server->sendWebSocketData(originalRequest->client, (char*)dt.c_str(), (int)dt.size(), true);
    }

}

void KWWebServerRouter::OnWebSocketDisconnect(shared_ptr<HttpData>originalRequest, string resource)
{

    try
    {
        auto index = std::find(connectedWebsockets.begin(), connectedWebsockets.end(), originalRequest->client);
        if (index != connectedWebsockets.end())
            connectedWebsockets.erase(index);

        identifyObservers("WS", originalRequest->resource, websocketRoutesObservers, [&](KWSRoute* item, map<string, string> vars){
            ((KWSWebsocketRouteExtended*)item)->getOnDisconnectObservers().foreach([&](int id, function<void(shared_ptr<HttpData>oriReq, map<string, string> varList)> curr){
                curr(originalRequest, vars);
            });
        });
    }
    catch(std::exception &e)
    {
        cerr << "Error disconnecting websocket: " << e.what() << endl;
    }
    catch(...)
    {
        cerr << "Error disconnecting websocket" << endl;
    }
}



bool KWWebServerRouter::identifyObservers(string method, string resource, map<string, vector<KWSRoute*>> routes, function<void(KWSRoute* item, map<string, string> vars)> action)
{
    string routeId = fixRouteId(method+resource);

    bool found = false;

    for(auto &currRoute: routes)
    {
        map<string, string> vars;
        if (routeIsCompatible(currRoute.first, routeId, vars))
        {
            extractUrlVars(resource, vars);
            for (auto &curr: currRoute.second)
            {
                found = true;
                action(curr, vars);
            }
        }

    }
    
    if (found)
        return true;

    if (!found && requestRoutesObservers.count("**"))
    {
        for (auto &curr: requestRoutesObservers["**"])
        {
            action(curr, {});
        }

        return true;
    }

    return false;

}

bool KWWebServerRouter::routeIsCompatible(string routeTemplate, string receivedResource, map<string, string> &vars)
{
    auto index = routeTemplate.find("["); 
    if (index != string::npos)
    {
        if (strUtils.toUpper(routeTemplate.substr(0, index)) == strUtils.toUpper(receivedResource.substr(0, index)))
        {
            //get varname
            routeTemplate = routeTemplate.substr(index+1);
            receivedResource = receivedResource.substr(index);

            auto varNameEndPos = routeTemplate.find("]");
            if (varNameEndPos != string::npos)
            {
                string varName = routeTemplate.substr(0, varNameEndPos);
                routeTemplate = routeTemplate.substr(varNameEndPos+1);


                auto varValuePos = receivedResource.find("/");
                if (varValuePos != string::npos)
                {
                    string varValue = receivedResource.substr(0, varValuePos);
                    vars[varName] = varValue;
                    receivedResource = receivedResource.substr(varValuePos);
                }
                else
                {
                    vars[varName] = receivedResource;
                    receivedResource = "";
                }

                return routeIsCompatible(routeTemplate, receivedResource, vars);
            }
            else
                //invalid route template
                return false;

        }
        else
            return false;
    }
    else
        return strUtils.toUpper(routeTemplate) == strUtils.toUpper(receivedResource);


}
 

void KWWebServerRouter::extractUrlVars(string url, map<string, string>& vars)
{
    if (url.find("?") != string::npos)
    {
        url = url.substr(url.find("?")+1);

        vector<string> varsAndValues = strUtils.split(url, "=");

        for (auto &c: varsAndValues)
        {
            if (c.find("=") != string::npos)
            {
                auto name = c.substr(c.find("="));
                auto value = c.substr(c.find("=")+1);
                vars[name] = value;
            }
            else if (c!= "")
            {
                vars[c] = "";
            }
        }
    }
}


string KWWebServerRouter::fixRouteId(string route)
{
    return route;
}

void KWWebServerRouter::runLocked(function<void()> f)
{
    listLockers.lock();
    f();
    listLockers.unlock();
}







void KWSWebsocketRoute::send(shared_ptr<HttpData>originalRequest, string data)
{
    this->send(originalRequest->client, data);
}

void KWSWebsocketRoute::send(shared_ptr<ClientInfo> client, string data)
{
    this->router->getServer()->sendWebSocketData(client, (char*)data.c_str(), (int)data.size(), true);

}

void KWSWebsocketRoute::sendToAll(string data)
{
    for (auto &c: this->router->getConnectedWebsockets())
        this->send(c, data);
}

size_t KWSWebsocketRoute::observersCount(){
    return router->getConnectedWebsockets().size();
}