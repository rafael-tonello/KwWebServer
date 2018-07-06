/*
*   By: Rafael Tonello (tonello.rafinha@gmail.com)
*   Version: 0.0.0.0 (09/08/2017)
*/
#include "KWTinyWebServer.h"
namespace Shared{

    KWTinyWebServer::KWTinyWebServer(int port, WebServerObserver *observer, vector<string> filesLocations)
    {
        this->__serverName = "Kiwiisco embeded server, version 0.0.0.0";
        this->__port = port;
        this->__observer = observer;
        this->__filesLocations = filesLocations;
        //ctor
        #ifdef __WIN32__
           WORD versionWanted = MAKEWORD(1, 1);
           WSADATA wsaData;
           WSAStartup(versionWanted, &wsaData);
        #endif

        pthread_create(&(this->ThreadAwaitClients), NULL, ThreadAwaitClientsFunction, this);
    }

    KWTinyWebServer::~KWTinyWebServer()
    {
        //dtor
    }

    void *ThreadAwaitClientsFunction(void *thisPointer)
    {
        KWTinyWebServer *self = (KWTinyWebServer*)thisPointer;

        //create an socket to await for connections

        int listener;

        struct sockaddr_in *serv_addr = new sockaddr_in();
        struct sockaddr_in *cli_addr = new sockaddr_in();
        int status;
        socklen_t clientSize;
        pthread_t *thTalkWithClient = NULL;



        listener = socket(AF_INET, SOCK_STREAM, 0);

        void ** tmp;

        if (listener >= 0)
        {
            //fill(std::begin(serv_addr), std::end(serv_addr), T{});
            //bzero((char *) &serv_addr, sizeof(serv_addr));
            //setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, (char *) &iOptVal, &iOptLen);

            serv_addr->sin_family = AF_INET;
            serv_addr->sin_addr.s_addr = INADDR_ANY;
            serv_addr->sin_port = htons(self->__port);

            usleep(1000);
            status = bind(listener, (struct sockaddr *) serv_addr, sizeof(*serv_addr));
            usleep(1000);
            if (status >= 0)
            {
                //SetSocketBlockingEnabled(listener, false);
                status = listen(listener, 5);
                if (status >= 0)
                {

                    clientSize = sizeof(cli_addr);
                    while (true)
                    {
			            usleep(1000);
                        int client = accept(listener, (struct sockaddr *) cli_addr, &clientSize);

                        if (client >= 0)
                        {
			                cout << "Client accepted: "<< client << endl << flush;
			                usleep(1000);
                            thTalkWithClient = new pthread_t;
                            tmp = new void*[3];
                            tmp[0] = self;
                            tmp[1] = &client;
                            tmp[2] = thTalkWithClient;

                            pthread_create(thTalkWithClient, NULL, ThreadTalkWithClientFunction, tmp);
                            //ThreadTalkWithClientFunction(tmp);
                        }
                    }
                }
                else
                    cout << "Failure to open socket" << endl << flush;
            }
            else
                cout << "Failure to start socket system" << endl << flush;
        }
        else
            cout << "General error opening socket" << endl << flush;

         //n = read(newsockfd,buffer,255);
         //n = write(newsockfd,"I got your message",18);
    }

    enum States {AWAIT_HEADER, PROCESS_HREADER, AWAIT_CONTENT, SEND_REQUEST_TO_APP, FINISH_REQUEST, FINISHED, ERROR_500_INTERNALSERVERERROR};

    void *ThreadTalkWithClientFunction(void *arguments)
    {
        void** params = (void**)arguments;
        KWTinyWebServer *self = (KWTinyWebServer*)params[0];
        int client = *((int*)(params[1]));
        pthread_t *thTalkWithClient = (pthread_t*)(params[2]);

        string internalServerErrorMessage = "";

        //SetSocketBlockingEnabled(client, false);
        HttpData receivedData, dataToSend;

        char *tempBuffer = NULL ;// = new char[2048];
        vector<char> rawBuffer;
        int readed = 1;

        char* tempIndStrConvert = NULL;

        string bufferStr = "";
        string headerSpliter = "";
        States state = AWAIT_HEADER;

        unsigned long headerEnd = 0;

        StringUtils strUtils;
        vector<string> headerLines;
        vector<string> tempHeaderParts;
        unsigned long ToRead = 0;

        int receiveTimeout = 10000;
        int contentStart;
        string temp;

        receivedData.httpStatus = 200;
        dataToSend.httpStatus = 200;

        int startTimeout = 2000;

        //waiting for webkit
        while (!__SocketIsConnected(client))
        {
            usleep(10000);
            startTimeout -= 10;
            if (startTimeout <= 0)
            {
                cout << "Browser doesn't stablished the connection"<<endl << flush;
                break;
            }
        }
        startTimeout = 2000;

        while ((state != FINISHED) && (__SocketIsConnected(client)))
        {
            #ifdef __WIN32__
            ioctlsocket(client, FIONREAD, &ToRead);
            #else
            ioctl(client,FIONREAD,&ToRead);
            #endif
            if (ToRead > 0)
            {
                tempBuffer = new char[ToRead+1];
                tempBuffer[ToRead] = 0;
                readed = recv(client, tempBuffer, ToRead, 0);//sizeof(char));

                if (readed > 0)
                {
                    //bufferStr.append(tempBuffer);
                    for (int cont = 0; cont < readed; cont++)
                    {
                        bufferStr += tempBuffer[cont];
                        rawBuffer.push_back(tempBuffer[cont]);
                        //read(newsockfd,buffer,255)
                    }
                }

                delete[] tempBuffer; 
                tempBuffer = NULL;
            }
            else
                usleep(20000);


            
            switch(state)
            {
                case AWAIT_HEADER:
                    debug = "WAIT HERADER \n";

                    //Try to find 
                    headerSpliter = "\r\n\r\n";
                    headerEnd = bufferStr.find(headerSpliter);

                    if (headerEnd != string::npos)
                    {
                        contentStart = headerEnd + headerSpliter.length();
                        state = PROCESS_HREADER;
                    }
                    else
                    {
                        usleep(100000);
                        startTimeout -= 100;
                        if (startTimeout <= 0)
                        {
                            cout << "Browser don't send headers" << endl << flush;
                            internalServerErrorMessage = "Browser don't send headers";
                            state = ERROR_500_INTERNALSERVERERROR;
                        }

                    }
                break;
                case PROCESS_HREADER:
                    debug += "process header \n";
                    //headerLines = strUtils.split(bufferStr, headerSpliter);
                    strUtils.split(bufferStr, headerSpliter, &headerLines);

                    //checkfs if any headr is received
                    if (headerLines.size() > 0)
                    {
                        //process first line, that contains method and resource
                        strUtils.split(headerLines[0], " ", &tempHeaderParts);
                        //checks if the first session line (GET /resource HTTP_Version) is valid
                        if (tempHeaderParts.size() > 1)
                        {
                            receivedData.method = tempHeaderParts.at(0);
                            receivedData.resource =tempHeaderParts.at(1);
                            receivedData.contentLength = 0;
                        }
                        else
                        {
                            internalServerErrorMessage = "Invalid HTTP request";
                            state = ERROR_500_INTERNALSERVERERROR;
                            break;
                        }
                    }
                    else
                    {
                        internalServerErrorMessage = "Invalid HTTP request";
                        state = ERROR_500_INTERNALSERVERERROR;
                        break;
                    }


                    //split anothers headers by ":"
                    for (unsigned int cont = 1; cont < headerLines.size(); cont++)
                    {
                        if (headerLines[cont] != "")
                        {
                            strUtils.split(headerLines.at(cont), ":", &tempHeaderParts);
                            if (tempHeaderParts.size() > 1)
                            {
                                receivedData.headers.push_back(tempHeaderParts);


                                if (strUtils.toUpper(tempHeaderParts.at(0)) == "CONTENT-LENGTH")
                                    receivedData.contentLength = atoi(tempHeaderParts.at(1).c_str());
                                else if (strUtils.toUpper(tempHeaderParts.at(0)) == "CONTENT-TYPE")
                                    receivedData.contentType = tempHeaderParts.at(1);
                            }

                            //#%%%%%%%%%% pode ser que esta fun��o limpe os dados que est�o indo apra o receivedData
                            tempHeaderParts.clear();
                        }
                        else
                            break;
                    }

                    state = AWAIT_CONTENT;
                    break;
                case AWAIT_CONTENT:
                    debug += "WAIT content \n";
                    if ((receivedData.contentLength == 0) || ((rawBuffer.size() - contentStart) >= receivedData.contentLength))
                    {
                        receivedData.contentBody = new char[(rawBuffer.size() - contentStart)+1];
                        receivedData.contentBody[(rawBuffer.size() - contentStart)] = 0;

                        for (unsigned int cont = contentStart; cont < rawBuffer.size(); cont++)
                        {
                            receivedData.contentBody[cont-contentStart] = rawBuffer[cont];
                        }
                        //put content body on receivedData

                        rawBuffer.clear();
                        state = SEND_REQUEST_TO_APP;
                    }
                    else
                        usleep(500);

                    break;
                case SEND_REQUEST_TO_APP: 
                    debug += "send request to app \n";
                    //try ausendFiles
                    self->__TryAutoLoadFiles(&receivedData, &dataToSend);

                    self->__observer->OnClientDataSend(&receivedData, &dataToSend);

                    //clear used data
                    delete[] receivedData.contentBody;

                    //mount response and send to client;
                    tempIndStrConvert = new char[10];


                    temp = "HTTP/1.1 "; temp.append(std::to_string(dataToSend.httpStatus)); temp.append(" "); temp.append(dataToSend.httpErrorMessage); temp.append("\r\n");
                    addStringToCharList(&rawBuffer, &temp, NULL, -1);

                    temp = "Server: "+self->__serverName+"\r\n";
                    addStringToCharList(&rawBuffer, &temp, NULL, -1);


                    /*temp = "Cache-Control: no-cache, no-store, must-revalidate";
                    addStringToCharList(&rawBuffer, &temp, NULL, -1);
                    temp = "Pragma: no-cache";
                    addStringToCharList(&rawBuffer, &temp, NULL, -1);
                    temp = "Expires: 0";
                    addStringToCharList(&rawBuffer, &temp, NULL, -1);*/
                    temp = "Connection: closed";
                    addStringToCharList(&rawBuffer, &temp, NULL, -1);


                    if (dataToSend.contentLength > 0)
                    {
                        temp = "Content-Length: "; temp.append(std::to_string(dataToSend.contentLength)); temp.append("\r\n");
                        addStringToCharList(&rawBuffer, &temp, NULL, -1);

                        temp = "Content-Type: "+dataToSend.contentType+";charset=utf-8\r\n";
                        addStringToCharList(&rawBuffer, &temp, NULL, -1);

                        temp = "Accept-Range: bytes\r\n";
                        addStringToCharList(&rawBuffer, &temp, NULL, -1);
                    }

                    temp = "Connection: Close\r\n";
                    addStringToCharList(&rawBuffer, &temp, NULL, -1);

                    //add custom headers
                    for (unsigned int cont = 0; cont < dataToSend.headers.size(); cont++)
                    {
                        if (dataToSend.headers.at(cont).size() > 1)
                        {
                            temp = dataToSend.headers.at(cont).at(0)+": "+dataToSend.headers.at(cont).at(1)+"\r\n";
                            addStringToCharList(&rawBuffer, &temp, NULL, -1);
                            //temp.clear();
                        }
                    }

                    //add end of header line break
                    temp = "\r\n";
                    addStringToCharList(&rawBuffer, &temp, NULL, -1);

                    //add content to buffer
                    for (unsigned int cont =0; cont < dataToSend.contentLength; cont++)
                        rawBuffer.push_back(dataToSend.contentBody[cont]);

                    //convert raw buffer to char*
                    tempBuffer = new char[rawBuffer.size()+1];
                    tempBuffer[rawBuffer.size()] = 0;
                    for (unsigned int cont =0; cont < rawBuffer.size(); cont++)
                        tempBuffer[cont] = rawBuffer.at(cont);


                    //write result to client
                    send(client, tempBuffer, rawBuffer.size(), 0);
                    rawBuffer.clear();
                    delete[] tempBuffer; tempBuffer = NULL;


                    //clear data
                    delete[] tempIndStrConvert; tempIndStrConvert = NULL;

                    if ((dataToSend.contentBody) && (dataToSend.contentLength > 0))
                    {

                        delete[] dataToSend.contentBody;
                        dataToSend.contentBody = NULL;
                    }
                    state = FINISH_REQUEST;
                    break;
                case FINISH_REQUEST:
                    state = FINISHED;
                    break;
                case ERROR_500_INTERNALSERVERERROR:
                    close(client);
                    state = FINISHED;
                    break;
            }

        }
        close(client);

        //headerLines.clear();
        //bufferStr.clear();
        delete[] params; params = NULL;
        if (tempBuffer)
        {
            delete[] tempBuffer; tempBuffer = NULL;
        }

        /*if (receivedData.contentBody)
        {
            //delete[] dataToSend.contentBody;
            receivedData.contentBody = new char[0];
        }*/

        if (tempIndStrConvert)
        {
            delete[] tempIndStrConvert; tempIndStrConvert = NULL;
        }

        /*for (unsigned int cont = 0; cont < receivedData.headers.size(); cont++)
            receivedData.headers.at(cont).clear();

        if (dataToSend.contentBody)
        {
            //delete[] dataToSend.contentBody;
            dataToSend.contentBody = new char[0];
        }
        for (unsigned int cont = 0; cont < dataToSend.headers.size(); cont++)
            dataToSend.headers.at(cont).clear();*/

        strUtils.~StringUtils();
        //delete &strUtils;
        //pthread_join(*thTalkWithClient, NULL);

        pthread_detach(*thTalkWithClient);
        pthread_exit(0);
    }

    void KWTinyWebServer::__TryAutoLoadFiles(HttpData* in, HttpData* out)
    {
        bool breakFors = false;
        vector<string> filesOnDirectory;
        StringUtils strUtils;
        string filePath;
        //determine the filename of httprequest 
        string fName = in->resource;

        if (fName=="/")
            fName = "index.html";


        string fNameUpper = strUtils.toUpper(fName);

        //remove '/' chrar from start of fName
        if (fName[0] == '/')
        {
            fName.erase(0, 1);
        }

        //scrols through the files locations
        for (int cont = 0; !breakFors && cont < this->__filesLocations.size(); cont++)
        {
            //list files from directory
            filePath = this->__filesLocations[cont] + "/"+ fName; 
            
            //scrols through the files of directories

            
            if (sysLink.fileExists(filePath))
            {
                //try to identificate the filetype

                if (fNameUpper.find(".GIF") != string::npos)
                    out->contentType = "image/gif";
                else if (fNameUpper.find(".JPG") != string::npos)
                    out->contentType = "image/jpeg";
                else if (fNameUpper.find(".PNG") != string::npos)
                    out->contentType = "image/png";
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
                out->contentLength = sysLink.getFileSize(filePath);

                out->contentBody = new char[out->contentLength];
                sysLink.readFile(filePath, out->contentBody, 0, out->contentLength);

                //stop find operations
                breakFors = true;

            }
        }
    }

    bool SetSocketBlockingEnabled(int fd, bool blocking)
    {
       if (fd < 0) return false;

        #ifdef _WIN32
           unsigned long mode = blocking ? 0 : 1;
           return (ioctlsocket(fd, FIONBIO, &mode) == 0) ? true : false;
        #else
           int flags = fcntl(fd, F_GETFL, 0);
           if (flags < 0) return false;
           flags = blocking ? (flags&~O_NONBLOCK) : (flags|O_NONBLOCK);
           return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
        #endif
    }

    void addStringToCharList(vector<char> *destination, string *source, char*source2, int source2Length)
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

    bool __SocketIsConnected( int socket)
    {
        char data;
        int readed = recv(socket,&data,1, MSG_PEEK | MSG_DONTWAIT);//read one byte (but not consume this)
        //return readed >= 0;

        int error_code;
        socklen_t error_code_size = sizeof(error_code);
        getsockopt(socket, SOL_SOCKET, SO_ERROR, &error_code, &error_code_size);
        string desc(strerror(error_code));

        if (error_code != 0)
            cout <<"ErrorCode: " << error_code << "("<< desc << ")" << endl << flush;

        return error_code == 0;

    }
}
