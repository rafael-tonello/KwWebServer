#include "KWTinyWebServer.h"
/*
*   By: Rafael Tonello (tonello.rafinha@gmail.com)
*   Version: 0.0.0.0 (09/08/2017)
*/

namespace KWShared
{
    using namespace std;

    void *ThreadWaitClients(void *thisPointer);
    void *HttpProcessThread(void *arguments);
    void *WebSocketProcessThread(void *arguments);
    bool SetSocketBlockingEnabled(int fd, bool blocking);
    void addStringToCharList(vector<char> *destination, string *source, char *source2, int source2Length = -1);
    bool __SocketIsConnected(int socket);
    string getFileETag(string fileName);

    KWTinyWebServer::KWTinyWebServer(
        int port, WebServerObserver *observer, 
        vector<string> filesLocations, 
        string dataFolder, 
        ThreadPool *tasker, 
        bool enableHttps,
        string sslKey,
        string sslPublicCert)
    {
        this->setServerInfo(this->_serverName+ ", version " + this->_serverVersion);

        if (tasker != NULL)
        {
            cout << "using a custom tasker for httpserver" << endl;
            this->__tasks = tasker;
        }
        else
            this->__tasks = new ThreadPool(5, 0, "KWBSrvrTsks");

        if (dataFolder == "_AUTO_DEFINE_" || dataFolder == "")
        {
            dataFolder = string(dirname((char *)(get_app_path().c_str()))) + "/data/www_data";
        }
        if (!this->sysLink.directoryExists(dataFolder))
            this->sysLink.createDirectory(dataFolder);
            
        if (this->__dataFolder.size() > 0 && this->__dataFolder[this->__dataFolder.size() - 1] != '/')
            this->__dataFolder += '/';

        this->__dataFolder = dataFolder;

        this->__observer = observer;
        this->__filesLocations = filesLocations;
//ctor
#ifdef __WIN32__
        WORD versionWanted = MAKEWORD(1, 1);
        WSADATA wsaData;
        WSAStartup(vers ionWanted, &wsaData);
#endif

        //start internal workers
        for (auto &curr : this->__workers)
            curr->start(this);

        //star the TCPServer
        bool tcpServerStartResult = false;
        if (enableHttps)
        {
            this->server = new TCPServer();
            auto initResult = this->server->startListen({
                shared_ptr<TCPServer_SocketInputConf>(new TCPServer_PortConf(port, "", true, sslKey, sslPublicCert))
            });
            tcpServerStartResult = initResult.startedPorts.size() > 0;
        }
        else
            this->server = new TCPServer(port, tcpServerStartResult);

        if (!tcpServerStartResult)
        {
            //cerr << "KwTinyWebServer startup error: the tcp server was not initialized correctly" << endl;
            throw std::runtime_error("KwTinyWebServer startup error: the tcp server was not initialized correctly");
            return;
        }

        this->server->addConEventListener(
            [this](shared_ptr<ClientInfo> client, CONN_EVENT event)
            {
                if (event == CONN_EVENT::CONNECTED)
                    this->initializeClient(client);
                else    
                    this->finalizeClient(client);
            }
        );

        this->server->addReceiveListener(
            [=](shared_ptr<ClientInfo> client, char *data, size_t dataSize)
            {
                if (this->clientsSessionsStates.count(client->socket) <= 0)
                {
                    //client->disconnect();
                    return;
                    //throw std::runtime_error("data received for a no intialized client");
                }

                auto clientState = this->clientsSessionsStates[client->socket];

                clientState->incomingDataLocker.lock();
                //apend incoming data to clientState->incomingDataBuffer
                for (size_t cont = 0; cont < dataSize; cont++)
                    clientState->incomingDataBuffer.push(data[cont]);
                

                if (!clientState->processingIncomingData)
                {
                    clientState->processingIncomingData = true;
                    
                    this->__tasks->enqueue([=](){
                        if (!server->isConnected(client)) { return; }

                        this->dataReceivedFrom(client);
                        clientState->processingIncomingData = false;
                    });
                }
                clientState->incomingDataLocker.unlock();
            }
        );
    }

    KWTinyWebServer::~KWTinyWebServer()
    {
        //dtor
        delete this->server;
    }

    void KWTinyWebServer::initializeClient(shared_ptr<ClientInfo> client)
    {
        if (clientsSessionsStates.count(client->socket) > 0)
            finalizeClient(client);

        auto tmp = std::make_shared<KWClientSessionState>();
        tmp->client = client;
        tmp->receivedData->client = client;
        tmp->dataToSend->client = client;
        tmp->state = READING_VERB;
        tmp->connection = "KEEP-ALIVE";
        tmp->processingIncomingData = false;

        clientsSessionsStatesMutex.lock();
        clientsSessionsStates[client->socket] = tmp;
        clientsSessionsStatesMutex.unlock();
    }

    void KWTinyWebServer::finalizeClient(shared_ptr<ClientInfo> client)
    {
        clientsSessionsStatesMutex.lock();
        if (clientsSessionsStates.count(client->socket) > 0)
        {
            if (clientsSessionsStates[client->socket]->webSocketOpen)
            {
                //finalize websocket
                clientsSessionsStates[client->socket]->webSocketState = WS_FINISHED;
                WebSocketProcess(client);
            }

            clientsSessionsStates[client->socket]->receivedData->clear();
            clientsSessionsStates[client->socket]->dataToSend->clear();

            //delete clientsSessionsStates[client->socket];
            clientsSessionsStates.erase(client->socket);

        }
        clientsSessionsStatesMutex.unlock();
    }

    void addStringToCharList(vector<char> *destination, string *source, char *source2, int source2Length)
    {
        if (source)
        {
            for (unsigned int cont = 0; cont < source->length(); cont++)
            {
                destination->push_back(source->at(cont));
            }
        }

        if (source2)
        {
            for (unsigned int cont = 0; ((source2Length == -1) && (source2[cont] != 0)) || (cont < source2Length); cont++)
                destination->push_back(source2[cont]);
        }
    }

    /*bool SetSocketBlockingEnabled(int fd, bool blocking)
	{
	   if (fd < 0) return false;

		#ifdef _WIN32
		   unsigned long modworker->start(this);e = blocking ? 0 : 1;
		   return (ioctlsocket(fd, FIONBIO, &mode) == 0) ? true : false;
		#else
		   int flags = fcntl(fd, F_GETFL, 0);
		   if (flags < 0) return false;
		   flags = blocking ? (flags&~O_NONBLOCK) : (flags|O_NONBLOCK);
		   return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
		#endif
	}*/

    void KWTinyWebServer::dataReceivedFrom(shared_ptr<ClientInfo> client)
    {
        const string validVerbCharacters="ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

        if (clientsSessionsStates.count(client->socket) <= 0)
        {
            cerr << "received data from a not initialized client (client not found)" << endl;
            return;
            //throw std::runtime_error("data received for a no intialized client");
        }

        shared_ptr<KWClientSessionState> sessionState = clientsSessionsStates[client->socket];

        if (sessionState == nullptr)
        {
            cerr << "received data from a not initialized client (client found but sessionState is null)" << endl;
            return;
            //throw std::runtime_error("data received for a no intialized client");
        }

        string keyUpper = "";
        char dtStr[256];
        time_t t;
        struct tm *timep;
        char tmpData;

        if (sessionState->webSocketOpen)
        {
            WebSocketProcess(client);

            return;
        }

        size_t cont = 0;

        while (
            (sessionState->state != SEND_REQUEST_TO_APP)
            && (sessionState->state != ERROR_400_BADREQUEST) 
            && (sessionState->state != ERROR_500_INTERNALSERVERERROR) 
            && (sessionState->state != ERROR_501_NOT_IMPLEMENTED) 
            && (sessionState->state != FINISHED) 
            && (sessionState->incomingDataBuffer.size() > 0)
        )
        {
            sessionState->prevState = sessionState->state;
            {
                switch (sessionState->state)
                {
                case READING_VERB:
                    sessionState->incomingDataLocker.lock();
                    tmpData = sessionState->incomingDataBuffer.front();
                    sessionState->incomingDataBuffer.pop();
                    sessionState->incomingDataLocker.unlock();

                    if (tmpData == ' ')
                    {
                        bool valid = true;
                        for (unsigned int cont2 = 0; cont2 < sessionState->bufferStr.size(); cont2++)
                        {
                            if (validVerbCharacters.find(sessionState->bufferStr[cont2]) == string::npos)
                            {
                                this->debug("Client sent a invalid verb: " + sessionState->bufferStr);
                                sessionState->internalServerErrorMessage = "Invalid HTTP request";
                                sessionState->state = ERROR_400_BADREQUEST;
                                sessionState->ignoreKeepAlive = true;
                                valid = false;
                                break;
                            }
                        }

                        if (valid){
                            sessionState->receivedData->method = sessionState->bufferStr;
                            //a header was received
                            sessionState->state = AWAIT_REMAIN_OF_HTTP_FIRST_LINE;
                            sessionState->bufferStr.clear();
                        }
                    }
                    //just a security to avoid a infinite loops.
                    else if (sessionState->bufferStr.size() > 30) 
                    {
                        //TODO: \026\003\001 may be the client trying to do a SSL connection
                        if (sessionState->bufferStr.find("\026\003\001") != string::npos)
                        {
                            this->debug("Maybe the client is trying to do a SSL connection. This instance of the HTTP server was not started with SSL support");
                            sessionState->internalServerErrorMessage = "SSL connection not supported because this instance of the server was not started with SSL support";
                            sessionState->state = ERROR_400_BADREQUEST;
                            sessionState->ignoreKeepAlive = true;
                        }
                        else
                        {
                            this->debug("Client sent a too long verb: " + sessionState->bufferStr);
                            sessionState->internalServerErrorMessage = "Invalid HTTP request";
                            sessionState->state = ERROR_400_BADREQUEST;
                            sessionState->ignoreKeepAlive = true;
                        }
                    }
                    else
                        sessionState->bufferStr += tmpData;
                
                    break;
                case AWAIT_REMAIN_OF_HTTP_FIRST_LINE:
                    sessionState->incomingDataLocker.lock();
                    tmpData = sessionState->incomingDataBuffer.front();
                    sessionState->incomingDataBuffer.pop();
                    sessionState->incomingDataLocker.unlock();

                    sessionState->bufferStr += tmpData;



                    if (sessionState->bufferStr.find("\r\n") != string::npos)
                    {
                        //a header was received
                        //process first line, that contains method and resource
                        sessionState->tempHeaderParts = StringUtils::split(sessionState->bufferStr, " ");

                        //checks if the first session line (GET /resource HTTP_Version) is valid
                        if (sessionState->tempHeaderParts.size() > 1)
                        {

                            
                            sessionState->receivedData->resource = sessionState->tempHeaderParts.at(0);
                            auto httpVersion = sessionState->tempHeaderParts.at(1);
                            sessionState->receivedData->contentLength = 0;
				
                            if (StringUtils::toUpper(httpVersion) != string("HTTP/1.1\r\n"))
                            {
                                this->debug("Client sent a invalid HTTP version: " + httpVersion);
                                sessionState->internalServerErrorMessage = "Invalid HTTP version (only HTTP/1.1 is supported)";
                                sessionState->state = ERROR_501_NOT_IMPLEMENTED;
                                sessionState->ignoreKeepAlive = true;
                                break;
                            }
                            
                        }
                        else
                        {
                            this->debug("Client sent a invalid HTTP request: " + sessionState->bufferStr);
                            sessionState->internalServerErrorMessage = "Invalid HTTP request";
                            sessionState->state = ERROR_500_INTERNALSERVERERROR;
                            sessionState->ignoreKeepAlive = true;
                            break;
                        }

                        sessionState->bufferStr.clear();

                        //start read headers
                        sessionState->state = READING_HEADER;
                    }

                    break;
                case READING_HEADER:
                    sessionState->incomingDataLocker.lock();
                    tmpData = sessionState->incomingDataBuffer.front();
                    sessionState->incomingDataBuffer.pop();
                    sessionState->incomingDataLocker.unlock();

                    sessionState->bufferStr += tmpData;
                    if (sessionState->bufferStr.find("\r\n") != string::npos)
                    {
                        if (sessionState->bufferStr != "\r\n")
                        {
                            //remote the "\r\n" from sessionState->bufferStr
                            sessionState->bufferStr.erase(sessionState->bufferStr.size() - 2, 2);
                            sessionState->tempHeaderParts = StringUtils::split(sessionState->bufferStr, ":");

                            //trim tempHeaderParts
                            for (auto &c : sessionState->tempHeaderParts)
                                c = StringUtils::trim(c);


                            //remove possible  ' ' from start of value
                            if (sessionState->tempHeaderParts.size() > 1 && sessionState->tempHeaderParts[1].size() > 0 && sessionState->tempHeaderParts[1][0] == ' ')
                                sessionState->tempHeaderParts[1].erase(0, 1);

                            if (sessionState->tempHeaderParts.size() > 1)
                            {
                                keyUpper = StringUtils::toUpper(sessionState->tempHeaderParts.at(0));
                                sessionState->tempHeaderParts[0] = keyUpper;

                                sessionState->receivedData->headers.push_back(sessionState->tempHeaderParts);

                                if (keyUpper == "CONTENT-LENGTH")
                                    sessionState->receivedData->contentLength = atoi(sessionState->tempHeaderParts.at(1).c_str());
                                else if (keyUpper == "CONTENT-TYPE")
                                    sessionState->receivedData->contentType = sessionState->tempHeaderParts.at(1);
                                else if (keyUpper == "ACCEPT")
                                    sessionState->receivedData->accept = sessionState->tempHeaderParts.at(1);
                                else if (keyUpper == "CONNECTION")
                                    sessionState->connection = StringUtils::toUpper(sessionState->tempHeaderParts.at(1));
                                else if (keyUpper == "UPGRADE")
                                    sessionState->upgrade = StringUtils::toUpper(sessionState->tempHeaderParts.at(1));
                            }

                            sessionState->tempHeaderParts.clear();
                        }
                        else
                        {
                            if (sessionState->receivedData->contentLength == 0)
                                sessionState->state = SEND_REQUEST_TO_APP;
                            else
                            {
                                sessionState->state = AWAIT_CONTENT;
                                
                                sessionState->receivedData->contentBody = shared_ptr<char[]>(new char[sessionState->receivedData->contentLength + 1]);
                                sessionState->receivedData->contentBody[sessionState->receivedData->contentLength] = 0;
                                sessionState->currentContentLength = 0;
                            }
                        }

                        sessionState->bufferStr.clear();
                    }

                    break;
                case AWAIT_CONTENT:
                    sessionState->incomingDataLocker.lock();
                    tmpData = sessionState->incomingDataBuffer.front();
                    sessionState->incomingDataBuffer.pop();
                    sessionState->incomingDataLocker.unlock();

                    sessionState->receivedData->contentBody[sessionState->currentContentLength++] = tmpData;

                    if (sessionState->currentContentLength == sessionState->receivedData->contentLength)
                    {
                        //rawBuffer.clear();
                        sessionState->state = SEND_REQUEST_TO_APP;
                    }
                    break;
                default:
                    break;
                }
                
            }
            cont++;
        }

        if (sessionState->connection.find("UPGRADE") != string::npos && sessionState->upgrade.find("WEBSOCKET") != string::npos)
        {
            sessionState->dataToSend->httpStatus = 101;
            sessionState->receivedData->httpStatus = 101;
        }

        //check if first part (read of headers and content) of the request is finished
        if (
            sessionState->state == READING_VERB
            || sessionState->state == AWAIT_REMAIN_OF_HTTP_FIRST_LINE
            || sessionState->state == READING_HEADER
            || sessionState->state == AWAIT_CONTENT
        )
        {
            return;
        }

        while (sessionState->state != FINISHED)
        {
            sessionState->prevState = sessionState->state;

            switch (sessionState->state)
            {
            case SEND_REQUEST_TO_APP:

                //set current time in the response
                t = time(0);
                timep = gmtime(&t);
                strftime(dtStr, 256, "%a, %d %b %Y %T GMT", timep);
                sessionState->dataToSend->headers.push_back({"date", string(dtStr)});
                //try autosendFiles
                if (sessionState->dataToSend->httpStatus != 101)
                    this->__TryAutoLoadFiles(sessionState->receivedData, sessionState->dataToSend);
                //notify workers
                for (auto &curr : this->__workers)
                    curr->load(sessionState->receivedData);

                //copy cookies and session data from receivedData to dataToSend
                for (auto &curr : sessionState->receivedData->cookies)
                {
                    if (sessionState->dataToSend->cookies.find(curr.first) != sessionState->dataToSend->cookies.end())
                        sessionState->dataToSend->cookies[curr.first]->copyFrom(curr.second);
                    else
                        sessionState->dataToSend->cookies[curr.first] = new HttpCookie(curr.second);
                }

                this->__observer->OnHttpRequest(sessionState->receivedData, sessionState->dataToSend);

                //clear used data

                if (sessionState->dataToSend->httpStatus == 0 && sessionState->dataToSend->contentType == "")
                {
                    sessionState->dataToSend->httpStatus = 404;
                    sessionState->dataToSend->httpMessage = "Not found";
                    sessionState->dataToSend->contentLength = 0;
                    sessionState->ignoreKeepAlive = true;
                }

                for (auto &curr : this->__workers)
                {
                    curr->unload(sessionState->dataToSend);
                }

                sessionState->state = CHECK_PROTOCOL_CHANGE;
                
                break;
            case CHECK_PROTOCOL_CHANGE:
                sessionState->state = SEND_RESPONSE;
                //check if http request is an protocol change request
                if (sessionState->receivedData->httpStatus == 101)
                {
                    sessionState->dataToSend->contentBody = shared_ptr<char[]>(new char[0]);
                    sessionState->dataToSend->contentLength = 0;

                    //WEBSOCKET CONNECTION (START ANOTHER THREAD TO WORK WITH CONNECTIN, SEND ACCEPTANCE TO CLIENT AND EXTI FROM HTTP PARSING PROCESS)
                    string secWebSocketKey = "";

                    //lock for SEC-WEBSOCKET-KEY key
                    for (auto &p : sessionState->receivedData->headers)
                    {
                        if (p[0] == "SEC-WEBSOCKET-KEY")
                            secWebSocketKey = p[1];
                    }

                    if (secWebSocketKey != "")
                    {
                        string concat = secWebSocketKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

                        unsigned char sha1result[SHA_DIGEST_LENGTH];
                        SHA1((unsigned char *)concat.c_str(), concat.size(), sha1result);
                        string secWebSocketBase64 = StringUtils::base64_encode(sha1result, SHA_DIGEST_LENGTH);

                        //prepare the http response to client
                        sessionState->dataToSend->httpStatus = 101;
                        sessionState->dataToSend->httpMessage = "Switching Protocols";
                        sessionState->dataToSend->setContentString("");
                        sessionState->dataToSend->headers.push_back({"Upgrade", "websocket"});
                        sessionState->dataToSend->headers.push_back({"Connection", "Upgrade"});
                        sessionState->dataToSend->headers.push_back({"Sec-WebSocket-Accept", secWebSocketBase64});

                        secWebSocketBase64.clear();
                        concat.clear();

                        auto cl = shared_ptr<HttpData>(new HttpData(sessionState->receivedData.get()));
                        sessionState->webSocketOpen = true;

                        sessionState->state = SEND_RESPONSE;
                    }
                    else
                        sessionState->state = ERROR_400_BADREQUEST;
                }
                else
                {
                    sessionState->dataToSend->headers.push_back({"Connection", "Close"});
                }
                break;
            case SEND_RESPONSE:

                //send first line and server name
                if (sessionState->dataToSend->httpStatus == 0)
                {
                    if (sessionState->dataToSend->contentLength == 0)
                    {
                        sessionState->dataToSend->httpStatus = 204;
                        sessionState->dataToSend->httpMessage = "No content";
                    }
                    else
                    {
                        sessionState->dataToSend->httpStatus = 200;
                        sessionState->dataToSend->httpMessage = "OK";
                    }

                }

                if (!server->isConnected(client)) { sessionState->state = FINISHED; endHttpRequest(client, sessionState); return; }
                client->sendString(
                    "HTTP/1.1 " +
                    std::to_string(sessionState->dataToSend->httpStatus) +
                    " " +
                    sessionState->dataToSend->httpMessage + 
                    "\r\n" +
                    "Server: " + this->getServerInfo() + "\r\n"
                );

                //send content identification headers
                if (sessionState->dataToSend->contentLength > 0)
                {
                    client->sendString(
                        "Content-Type: " + sessionState->dataToSend->contentType + ";charset=utf-8\r\n" +
                        "Accept-Range: bytes\r\n" +
                        "Content-Length: " +
                        std::to_string(sessionState->dataToSend->contentLength) + 
                        "\r\n"
                    );

                }

                //send custom headers
                for (unsigned int cont = 0; cont < sessionState->dataToSend->headers.size(); cont++)
                {
                    if (sessionState->dataToSend->headers.at(cont).size() > 1)
                    {
                        //TODO: if you keep f5 pressed on browser, this line raises an error
                        client->sendString(sessionState->dataToSend->headers.at(cont).at(0) + ": " + sessionState->dataToSend->headers.at(cont).at(1) + "\r\n");
                    }
                }

                //add end of header line break
                client->sendString("\r\n");

                //add content to buffer
                client->sendData(sessionState->dataToSend->contentBody.get(), sessionState->dataToSend->contentLength);

                //convert raw buffer to char*

                sessionState->dataToSend->clear();

                sessionState->state = FINISH_REQUEST;

                break;
            case FINISH_REQUEST:
                sessionState->state = FINISHED;
                break;
            case ERROR_400_BADREQUEST:
                client->sendString(
                    string("HTTP/1.1 400 Bad request \r\n") +
                    string("Server: ") + this->getServerInfo() + string("\r\n")
                );
                sessionState->state = FINISHED;
                break;
            case ERROR_500_INTERNALSERVERERROR:
                client->sendString(
                    "HTTP/1.1 500 "+sessionState->internalServerErrorMessage+" \r\n" +
                    "Server: " + this->getServerInfo() + "\r\n"
                );
                sessionState->state = FINISHED;
                break;
            case ERROR_501_NOT_IMPLEMENTED:
                client->sendString(
                    "HTTP/1.1 501 "+sessionState->internalServerErrorMessage+" \r\n" +
                    "Server: " + this->getServerInfo() + "\r\n"
                );
                sessionState->state = FINISHED;
                break;
            default:
                client->sendString(
                    "HTTP/1.1 500 Server entered in a invalid state: "+to_string((int)sessionState->state)+" \r\n" +
                    "Server: " + this->getServerInfo() + "\r\n"
                );
                sessionState->state = FINISHED;
                break;
            }
            cont++;
        }

        if (sessionState->state == FINISHED)
        {
            this->endHttpRequest(client, sessionState);
        }
    }

    void KWTinyWebServer::endHttpRequest(shared_ptr<ClientInfo> client, shared_ptr<KWClientSessionState> sessionState)
    {
        if (sessionState->webSocketOpen)
            this->__observer->OnWebSocketConnect((sessionState->receivedData), sessionState->receivedData->resource);
        else
        {
            if (isToKeepAlive(sessionState) && !sessionState->ignoreKeepAlive)
            {
                //delete sessionState;
                finalizeClient(client);
                initializeClient(client);
                sessionState = clientsSessionsStates[client->socket];
            }
            else
            {
                if (!server->isConnected(client)) { return; }
                client->disconnect();
                //Client is finalized when TCPServer emits a event about the client disconnection (see this in the constructor)
            }

        }
    }

    bool KWTinyWebServer::isToKeepAlive(shared_ptr<KWClientSessionState> sessionState)
    {
        return sessionState->connection.find("CLOSE") == string::npos;
    }

    void KWTinyWebServer::WebSocketProcess(shared_ptr<ClientInfo> client)
    {
        auto sessionState = this->clientsSessionsStates[client->socket];

        shared_ptr<HttpData>request = (sessionState->receivedData);
        string resource = request->resource;

        /*int readCount;
        char readBuffer[1024];

        //packinfo data
        bool resv3;



        */

        //some problems to websocket connection was solved with this delay
        //usleep(2000000);

        
        char ws_curr;

        for (size_t cont = 0; sessionState->incomingDataBuffer.size() > 0 && sessionState->webSocketState != WS_FINISHED; cont++)
        {
            sessionState->incomingDataLocker.lock();
            ws_curr = sessionState->incomingDataBuffer.front();
            sessionState->incomingDataBuffer.pop();
            sessionState->incomingDataLocker.unlock();
            //READ DATA FROM SOCKET

            switch (sessionState->webSocketState)
            {
            //read the first 2 bytes that contains the information about the incoming pack
            case WS_READING_PACK_INFO_1:
                sessionState->ws_tempIndex = 0;
                sessionState->ws_totalPayload = 0;
                //extract bit finformation
                //fin indicates that  this pack is the last of the current message (message can be made of many packs)
                sessionState->ws_fin = ws_curr & 0b10000000;

                //the opcode is the type of pack (0x00 is a continuatino pack, 0x01 is a text pack, 0x02 is a binary pack)
                sessionState->ws_opcode = ws_curr & 0b00001111;

                sessionState->ws_tempIndex = 0;

                //clear the buffer
                if (sessionState->ws_packPayload)
                {
                    sessionState->ws_packPayload = NULL;client->disconnect();
        
                }

                sessionState->webSocketState = WS_READING_PACK_INFO_2;
                break;
            case WS_READING_PACK_INFO_2:
                //mask indicates if the pack data is masked. Any messages came from client must be masked. If this field has value 0,
                //the serve must send an error information and closes the socket connection
                sessionState->ws_masked = ws_curr & 0b10000000;
                sessionState->ws_mask[0] = 0;
                sessionState->ws_mask[1] = 0;
                sessionState->ws_mask[2] = 0;
                sessionState->ws_mask[3] = 0;

                if (sessionState->ws_masked == 1)
                {
                    //read the first byt of packSize. If this value is equals to 126, the server needs to read the next 16 bits to get an
                    //unsigned 16 bit int with the pack sie. If this value is equals to 127, the server must read the next 64 bits to get
                    //an unsigned int64 with the pack size. If the value is less than 126, this is the pack size
                    sessionState->ws_packSize7bit = ws_curr & 0b01111111;
                    if (sessionState->ws_packSize7bit < 126)
                    {
                        sessionState->ws_packSize = sessionState->ws_packSize7bit;
                        sessionState->ws_tempIndex = 0;
                        sessionState->webSocketState = WS_READING_MASK_KEY;
                    }
                    else if (sessionState->ws_packSize == 126)
                        sessionState->webSocketState = WS_READ_16BIT_SIZE;
                    else if (sessionState->ws_packSize == 127)
                        sessionState->webSocketState = WS_READ_64BIT_SIZE;
                    else
                    {
                        //This code never can be executed. If execution reaches this code, the server was erroneus readed  the packSize
                        //or errnoeus made bit-to-bit process.
                        cont--;
                        sessionState->webSocketState = WS_INTERNAL_SERVER_ERROR;
                    }

                    sessionState->ws_tempIndex = 0;
                    sessionState->ws_packSize16bit = 0;
                }
                else
                {
                    this->debug("Error, websocket payload not masked");
                    cont--;
                    sessionState->webSocketState = WS_PAYLOAD_NOT_MASKED;

                    //although it goes against the specification of WebSocket, it is possible to interpret unmarked packages from the
                    //client, if this is decided, just ignore the reading of the mask and go straight to the reading of the payload.
                }
                break;
            case WS_READ_16BIT_SIZE:
                ((char *)&(sessionState->ws_packSize16bit))[sessionState->ws_tempIndex++] = ws_curr;
                if (sessionState->ws_tempIndex == 2)
                {
                    sessionState->ws_packSize = sessionState->ws_packSize16bit;
                    sessionState->ws_tempIndex = 0;
                    sessionState->webSocketState = WS_READING_MASK_KEY;
                }
                break;
            case WS_READ_64BIT_SIZE:

                ((char *)&(sessionState->ws_packSize))[sessionState->ws_tempIndex++] = ws_curr;
                if (sessionState->ws_tempIndex == 4)
                {
                    sessionState->ws_tempIndex = 0;
                    sessionState->webSocketState = WS_READING_MASK_KEY;
                }

                break;
            case WS_READING_MASK_KEY:
                sessionState->ws_mask[sessionState->ws_tempIndex++] = ws_curr;
                if (sessionState->ws_tempIndex == 4)
                {
                    sessionState->ws_tempIndex = 0;
                    sessionState->webSocketState = WS_READING_PAYLOAD;
                }
                break;
            case WS_READING_PAYLOAD:
                if (!sessionState->ws_packPayload)
                    sessionState->ws_packPayload = new char[sessionState->ws_packSize];

                if (sessionState->ws_tempIndex < sessionState->ws_packSize)
                {
                    sessionState->ws_packPayload[sessionState->ws_tempIndex] = ws_curr ^ sessionState->ws_mask[sessionState->ws_tempIndex % 4];
                    sessionState->ws_tempIndex++;
                }

                //don't use else here, becauze tempIndex is changed on 'if' above and can't enter at code bellow in te last pack byte.

                if (sessionState->ws_tempIndex >= sessionState->ws_packSize)
                {
                    sessionState->ws_payload.push_back(sessionState->ws_packPayload);
                    sessionState->ws_payloadSizes.push_back(sessionState->ws_tempIndex);
                    sessionState->ws_totalPayload += sessionState->ws_tempIndex;

                    //check if pack is the last of message
                    if (sessionState->ws_fin)
                    {
                        sessionState->ws_tempIndex = 0;
                        //parepare an object to contaisn the received data

                        char *fullPayload = new char[sessionState->ws_totalPayload];

                        //get all data from all framees of message
                        for (size_t cPayload = 0; cPayload < sessionState->ws_payload.size(); cPayload++)
                        {
                            for (int cData = 0; cData < sessionState->ws_payloadSizes[cPayload]; cData++)
                            {
                                fullPayload[sessionState->ws_tempIndex++] = sessionState->ws_payload[cPayload][cData];
                            }

                            delete[] sessionState->ws_payload[cPayload];
                        }

                        sessionState->ws_payload.clear();
                        sessionState->ws_payloadSizes.clear();
                        //send payload to application

                        this->__observer->OnWebSocketData(request, resource, fullPayload,sessionState->ws_totalPayload);

                        //clear payload buffers
                        delete[] fullPayload;
                        sessionState->ws_totalPayload = 0;
                    }
                    delete []sessionState->ws_packPayload;
                    sessionState->ws_packPayload = NULL;

                    sessionState->ws_tempIndex = 0;

                    sessionState->webSocketState = WS_READING_PACK_INFO_1;
                }

                break;
                case WS_INTERNAL_SERVER_ERROR:
                    sessionState->webSocketState = WS_FINISHED;
                break;
                case WS_PAYLOAD_NOT_MASKED:
                    sessionState->webSocketState = WS_FINISHED;
                break;
                default:
                    break;
            }
        }

        if (sessionState->webSocketState == WS_FINISHED)
        {
            this->__observer->OnWebSocketDisconnect(request, resource);
            client->disconnect();
            request->clear();
        }
    }

    void KWTinyWebServer::__TryAutoLoadFiles(shared_ptr<HttpData>in, shared_ptr<HttpData>out)
    {
        bool breakFors = false;
        vector<string> filesOnDirectory;
        StringUtils strUtils;
        string filePath;
        //determine the filename of httprequest
        string fName = in->resource;

        if (fName == "/")
            fName = "index.html";

        string fNameUpper = StringUtils::toUpper(fName);

        //remove '/' chrar from start of fName
        if (fName[0] == '/')
        {
            fName.erase(0, 1);
        }

        //scrols through the files locations
        for (size_t cont = 0; !breakFors && cont < this->__filesLocations.size(); cont++)
        {
            //list files from directory
            filePath = this->__filesLocations[cont] + "/" + fName;

            if (sysLink.fileExists(filePath))
            {
                //try to identificate the filetype
                //gets the lats file modification (this information will be used a few times above)
                struct stat attrib;
                struct tm *time;
                char dtStr[256];
                stat(filePath.c_str(), &attrib);
                time = gmtime(&(attrib.st_mtime));
                strftime(dtStr, 256, "%a, %d %b %Y %T GMT", time);
                string lastModificationTime(dtStr);
                string ETag = StringUtils::base64_encode((unsigned char *)lastModificationTime.c_str(), lastModificationTime.size());

                //checks if browseris just browser is just checking by modifications in the resource
                string ifNoneMatchHeader = "";
                for (auto &curr : in->headers)
                {
                    if (curr.at(0) == "IF-NONE-MATCH")
                    {
                        ifNoneMatchHeader = curr.at(1);
                        break;
                    }
                }

                if (ifNoneMatchHeader == "" || ifNoneMatchHeader != ETag)
                {

                    if (fNameUpper.find(".GIF") != string::npos)
                        out->contentType = "image/gif";
                    else if (fNameUpper.find(".JPG") != string::npos)
                        out->contentType = "image/jpeg";
                    else if (fNameUpper.find(".SVG") != string::npos)
                        out->contentType = "image/svg+xml";
                    else if (fNameUpper.find(".PNG") != string::npos)
                        out->contentType = "image/png";
                    else if (fNameUpper.find(".ICO") != string::npos)
                        out->contentType = "image/ico";
                    else if (fNameUpper.find(".HTM") != string::npos)
                        out->contentType = "text/html";
                    else if (fNameUpper.find(".CSS") != string::npos)
                        out->contentType = "text/css";
                    else if (fNameUpper.find(".JS") != string::npos)
                        out->contentType = "text/javascript";

                    else if (fNameUpper.find(".JSON") != string::npos)
                        out->contentType = "application/json";
                    else
                        out->contentType = "text/plain";

                    //load the file ontent
                    {
                        out->contentLength = sysLink.getFileSize(filePath);
                        
                        out->contentBody = shared_ptr<char[]>(new char[out->contentLength]);
                        sysLink.readFile(filePath, out->contentBody.get(), 0, out->contentLength);
                    }

                    //set the cache headers
                    {
                        //out->headers.push_back({"cache-control", "public, max-age=3600"});
                        out->headers.push_back({"cache-control", "public, max-age=3600, no-cache, must-revalidate"});

                        //get file last modification time

                        out->headers.push_back({"last-modified", lastModificationTime});

                        out->headers.push_back({"ETag", ETag});
                        //set the ETag header
                    }
                    out->httpStatus = 200;
                    out->httpMessage = "Ok";
                }
                else
                {
                    //just prepare a 304 (not modified) response
                    out->httpStatus = 304;
                    out->httpMessage = "Not Modified";
                }

                //stop find operations
                breakFors = true;
            }
            else
            {
                out->contentType = "";
                out->contentLength = 0;
                out->httpStatus = 0;

                string t = "the file " + fName + "could not be found ";
                debug(t);
            }
        }
    }

    void KWTinyWebServer::sendWebSocketData(shared_ptr<ClientInfo> cli, char *data, int size, bool isText)
    {
        char headerBuffer[14]; // O buffer de cabeçalho é grande o suficiente para acomodar o cabeçalho e tamanho máximo da carga útil.
        int headerSize = 2; // Tamanho inicial do cabeçalho (sem estender para tamanhos maiores de carga útil).

        // Defina o bit FIN (finalização) como verdadeiro (1).
        headerBuffer[0] = 0b10000000;

        // Defina o opcode (0x01 para dados de texto, 0x02 para dados binários).
        headerBuffer[0] |= isText ? 0x01 : 0x02;

        // Defina o bit MASK como 0, indicando que os dados não são mascarados.
        headerBuffer[1] = size < 126 ? size : 126;

        if (size >= 126)
        {
            if (size < 65536)
            {
                headerBuffer[1] = 126; // Indique que o tamanho é maior que 125 bytes.
                uint16_t size16 = static_cast<uint16_t>(size);
                headerBuffer[2] = (size16 >> 8) & 0xFF;
                headerBuffer[3] = size16 & 0xFF;
                headerSize += 2;
            }
            else
            {
                headerBuffer[1] = 127; // Indique que o tamanho é maior que 65535 bytes.
                uint64_t size64 = static_cast<uint64_t>(size);
                for (int i = 2; i < 10; i++)
                {
                    headerBuffer[i] = (size64 >> ((9 - i) * 8)) & 0xFF;
                }
                headerSize += 8;
            }
        }

        // Envie o cabeçalho para o cliente.
        cli->sendData(headerBuffer, headerSize);

        // Envie os dados da carga útil.
        cli->sendData(data, size);
    }

    void KWTinyWebServer::sendWebSocketData(shared_ptr<HttpData>originalRequest, char *data, int size, bool isText)
    {
        this->sendWebSocketData(originalRequest->client, data, size, isText);
    }

    void KWTinyWebServer::broadcastWebSocker(char* data, int size, bool isText, string resource)
    {

        vector<shared_ptr<ClientInfo>> cli;
        clientsSessionsStatesMutex.lock();
        for (auto &c: clientsSessionsStates)
        {
            if (c.second->webSocketOpen == true)
            {
                if (resource == "*" || resource == "" || c.second->receivedData->resource == resource)
                    cli.push_back(c.second->client);
            }
        }
        clientsSessionsStatesMutex.unlock();


        for (auto &c: cli)
        {
            this->sendWebSocketData(c, data, size, isText);
        }

    }

    void KWTinyWebServer::disconnecteWebSocket(shared_ptr<ClientInfo> client)
    {
        client->disconnect();
    }

    void KWTinyWebServer::disconnecteWebSocket(shared_ptr<HttpData> originalRequest)
    {
        this->disconnecteWebSocket(originalRequest->client);
    }

    void KWTinyWebServer::debug(string debugMessage, bool forceFlush)
    {
        /*
        cout << "WebServer: " << debugMessage << endl;
        if (forceFlush)
            cout << flush;*/
    }

    long int KWTinyWebServer::getCurrDayMilisec()
    {

        struct timeval temp;
        gettimeofday(&temp, NULL);

        return temp.tv_sec * 1000 + temp.tv_usec / 1000;
    }

    string KWTinyWebServer::get_app_path()
    {
        char arg1[200];
        char exepath[PATH_MAX + 1] = {0};

        sprintf(arg1, "/proc/%d/exe", getpid());
        readlink(arg1, exepath, 1024);
        return string(exepath);
    }

    void KWTinyWebServer::addWorker(IWorker *worker)
    {
        this->__workers.push_back(worker);
        worker->start(this);
    }
}
