#include "KWTinyWebServer.h"
/*
*   By: Rafael Tonello (tonello.rafinha@gmail.com)
*   Version: 0.0.0.0 (09/08/2017)
*/

namespace KWShared{
    using namespace std;


    void *ThreadWaitClients(void *thisPointer);
    void *HttpProcessThread(void *arguments);
    void *WebSocketProcessThread(void *arguments);
    bool SetSocketBlockingEnabled(int fd, bool blocking);
    void addStringToCharList(vector<char> *destination, string *source, char*source2, int source2Length = -1);
    bool __SocketIsConnected( int socket);
	string getFileETag(string fileName);

	KWTinyWebServer::~KWTinyWebServer()
	{
		//dtor
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

		int error_code;
		socklen_t error_code_size = sizeof(error_code);
		getsockopt(socket, SOL_SOCKET, SO_ERROR, &error_code, &error_code_size);
		string desc(strerror(error_code));


		return error_code == 0;

	}

	bool SetSocketBlockingEnabled(int fd, bool blocking)
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
	}

	void *HttpProcessThread(void *arguments)
	{
		void** params = (void**)arguments;
		KWTinyWebServer *self = (KWTinyWebServer*)params[0];
		int client = ((int)(params[1]));

		delete [] params;
		params = NULL;

		//SetSocketBlockingEnabled(client, true);

		string internalServerErrorMessage = "";
		HttpData receivedData, dataToSend;
		receivedData.client = client;
		dataToSend.client = client;
		//char *tempBuffer = NULL;
		char readBuffer[2048];
		char writeBuffer[2048];
		char * tempBuffer2  = NULL ;
		vector<char> rawBuffer;
		int readed = 1;
		char* tempIndStrConvert = NULL;
		string bufferStr = "";
		string headerSpliter = "";
		States state = AWAIT_HTTP_FIRST_LINE;
		States prevState = AWAIT_HTTP_FIRST_LINE;
		unsigned long headerEnd = 0;
		StringUtils strUtils;
		//vector<string> headerLines;
		vector<string> tempHeaderParts;
		unsigned long ToRead = 0;
		int receiveTimeout = 1000;
		int contentStart;
		string temp;
		receivedData.httpStatus = 200;
		dataToSend.httpStatus = 200;
		int timeoutCounter = 2000;
		unsigned int sendSize;
		int writed = 0, tempWrited = 0;
		int cBytes;
		string keyUpper = "";
		string connection = "";
		string upgrade = "";
		unsigned int currentContentLength;

		//the flag bellow is used  when a websocket connection is detected to don't close the socket at end of HttpProcessThread thread.
		bool letSocketOpen  = false;
		char dtStr[256];
        time_t t;
        struct tm *timep;
        long int startTime = self->getCurrDayMilisec();

		//waiting for webkit

		while (!__SocketIsConnected(client))
		{
			usleep(1000);
			timeoutCounter -= 1;
			if (timeoutCounter <= 0)
			{
				self->debug("Browser doesn't stablished the connection", true);
				break;
			}
		}
		//set read timeout
		struct timeval tv;
		tv.tv_sec  = 10;
		tv.tv_usec = 0;

		setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

		timeoutCounter = 5000;


		while ((state != SEND_REQUEST_TO_APP) && (state != ERROR_500_INTERNALSERVERERROR) && (state != FINISHED) && (__SocketIsConnected(client)))
		{
			if (prevState != state)
			{
                startTime = self->getCurrDayMilisec();
			}
			else{
                if (self->getCurrDayMilisec() - startTime > 5000)
                {
                    state = FINISHED;
                    self->debug("A connection will be closed by timeout");
                    continue;
                }
			}

			prevState = state;

			//tempBuffer = new char[2048];
			readed = recv(client,readBuffer, sizeof(readBuffer), 0);


			if (readed > 0)
			{
				//bufferStr.append(tempBuffer);
				for (int cont = 0; cont < readed; cont++)
				{
					switch(state)
					{
						case AWAIT_HTTP_FIRST_LINE:
							bufferStr += readBuffer[cont];
							if (bufferStr.find("\r\n") != string::npos)
							{
								//a header was received
								//process first line, that contains method and resource
                                strUtils.split(bufferStr, " ", &tempHeaderParts);

								//checks if the first session line (GET /resource HTTP_Version) is valid
								if (tempHeaderParts.size() > 1)
								{

									receivedData.method = tempHeaderParts.at(0);
									receivedData.resource = tempHeaderParts.at(1);
									receivedData.contentLength = 0;
								}
								else
								{
									internalServerErrorMessage = "Invalid HTTP request";
									state = ERROR_500_INTERNALSERVERERROR;
									break;
								}


								bufferStr.clear();

								//start read headers
								state = READING_HEADER;
							}

						break;
						case READING_HEADER:
							bufferStr += readBuffer[cont];
							if (bufferStr.find("\r\n") != string::npos)
							{
								if (bufferStr != "\r\n")
								{
                                    //remote the "\r\n" from bufferStr
                                    bufferStr.erase(bufferStr.size()-2, 2);
									strUtils.split(bufferStr, ":", &tempHeaderParts);

									//remove possible  ' ' from start of value
									if (tempHeaderParts[1].size() > 0 && tempHeaderParts[1][0] == ' ')
                                        tempHeaderParts[1].erase(0, 1);

									if (tempHeaderParts.size() > 1)
									{
										keyUpper = strUtils.toUpper(tempHeaderParts.at(0));
										tempHeaderParts[0] = keyUpper;

										receivedData.headers.push_back(tempHeaderParts);

										if (keyUpper == "CONTENT-LENGTH")
											receivedData.contentLength = atoi(tempHeaderParts.at(1).c_str());
										else if (keyUpper == "CONTENT-TYPE")
											receivedData.contentType = tempHeaderParts.at(1);
										else if (keyUpper == "CONNECTION")
											connection = strUtils.toUpper(tempHeaderParts.at(1));
										else  if (keyUpper == "UPGRADE")
											upgrade = strUtils.toUpper(tempHeaderParts.at(1));
									}

									//#%%%%%%%%%% pode ser que esta fun��o limpe os dados que est�o indo apra o receivedData
									tempHeaderParts.clear();
								}
								else
								{
									if (receivedData.contentLength == 0)
										state = SEND_REQUEST_TO_APP;
									else
									{
										state = AWAIT_CONTENT;
										receivedData.contentBody = new char[(rawBuffer.size() - contentStart)+1];
										receivedData.contentBody[(rawBuffer.size() - contentStart)] = 0;
										currentContentLength = 0;
									}
								}

								bufferStr.clear();
							}

							break;
						case AWAIT_CONTENT:
							receivedData.contentBody[currentContentLength++] = readBuffer[cont];

							if (currentContentLength == receivedData.contentLength)
							{
								rawBuffer.clear();
								state = SEND_REQUEST_TO_APP;
							}
							else
								usleep(500);

							break;
					}
				}
			}
			else
			{
                //conection was closed by remote end point
                state  = FINISHED;
            }

		}

        //check if http request is an protocol change request
        if (connection.find("UPGRADE") != string::npos && upgrade.find("WEBSOCKET") != string::npos)
        {
            //WEBSOCKET CONNECTION (START ANOTHER THREAD TO WORK WITH CONNECTIN, SEND ACCEPTANCE TO CLIENT AND EXTI FROM HTTP PARSING PROCESS)
            string secWebSocketKey = "";

            //lock for SEC-WEBSOCKET-KEY key
            for (auto &p : receivedData.headers)
            {
                if (p[0] == "SEC-WEBSOCKET-KEY")
                    secWebSocketKey = p[1];
            }


            if (secWebSocketKey != "")
            {
                string concat = secWebSocketKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";



                unsigned char sha1result[SHA_DIGEST_LENGTH];
                SHA1((unsigned char*)concat.c_str(), concat.size(), sha1result);
                string secWebSocketBase64 = self->_strUtils.base64_encode(sha1result, SHA_DIGEST_LENGTH);




                //prepare the http response to client
                dataToSend.httpStatus = 101;
                dataToSend.httpMessage = "Switching Protocols";
                dataToSend.headers.push_back({"Upgrade", "websocket"});
                dataToSend.headers.push_back({"Connection", "Upgrade"});
                dataToSend.headers.push_back({"Sec-WebSocket-Accept", secWebSocketBase64});

                secWebSocketBase64.clear();
                concat.clear();



                state = SEND_RESPONSE;

                //create a new thread to work with the web socket connection
                pthread_t *thWebSocketProcess = new pthread_t;
                void **tmp = new void*[4];
                HttpData* cl = new HttpData(receivedData);
                tmp[0] = self;
                tmp[1] = &client;
                tmp[2] = thWebSocketProcess;
				tmp[3] = cl;

                pthread_create(thWebSocketProcess, NULL, WebSocketProcessThread, tmp);

                letSocketOpen = true;

                //this time must be enough to new thread be started. It's just a prediction, ensuring that the new thread
                //has access to the current thread variables.
                usleep(100000);

            }
            else
                state = ERROR_400_BADREQUEST;


        }
        else{
            dataToSend.headers.push_back({"Connection", "Close"});
        }

		while ((state != FINISHED) && (__SocketIsConnected(client)))
		{
            if (prevState != state)
			{
                startTime = self->getCurrDayMilisec();
			}
			else{
                if (self->getCurrDayMilisec() - startTime > 5000)
                {
                    state = FINISHED;
                    self->debug("A connection will be closed by timeout");
                    continue;
                }
			}

			prevState = state;

			switch(state)
			{
				case SEND_REQUEST_TO_APP:
                    //set current time in the response
                    t = time(0);
                    timep = gmtime(&t);
                    strftime(dtStr, 256, "%a, %d %b %Y %T GMT", timep);
                    dataToSend.headers.push_back({"date", string(dtStr)});


					//try ausendFiles
					self->__TryAutoLoadFiles(&receivedData, &dataToSend);
					//notify workers
					for (auto &curr: self->__workers)
                        curr->load(&receivedData);

                    //copy cookies and session data from receivedData to dataToSend
                    for (auto &curr: receivedData.cookies)
                    {
                        if (dataToSend.cookies.find(curr.first) != dataToSend.cookies.end())
                            dataToSend.cookies[curr.first]->copyFrom(curr.second);
                        else
                            dataToSend.cookies[curr.first] = new HttpCookie(curr.second);
                    }

					self->__observer->OnHttpRequest(&receivedData, &dataToSend);

					//clear used data
					delete[] receivedData.contentBody;
					receivedData.contentBody = NULL;
					//mount response and send to client;
					tempIndStrConvert = new char[10];

					if (dataToSend.contentType == "notFound")
					{
						dataToSend.httpStatus = 404;
						dataToSend.httpMessage = "Not found";
						dataToSend.contentLength = 0;
					}

					for (auto &curr: self->__workers)
					{
                        curr->unload(&dataToSend);
                    }


					state = SEND_RESPONSE;
                break;
                case SEND_RESPONSE:
					temp = "HTTP/1.1 "; temp.append(std::to_string(dataToSend.httpStatus)); temp.append(" "); temp.append(dataToSend.httpMessage); temp.append("\r\n");
					addStringToCharList(&rawBuffer, &temp, NULL, -1);

					temp = "Server: "+self->__serverName+"\r\n";
					addStringToCharList(&rawBuffer, &temp, NULL, -1);
					if (dataToSend.contentLength > 0)
					{
						temp = "Content-Type: "+dataToSend.contentType+";charset=utf-8\r\n";
						addStringToCharList(&rawBuffer, &temp, NULL, -1);

						temp = "Accept-Range: bytes\r\n";
						addStringToCharList(&rawBuffer, &temp, NULL, -1);

						temp = "Content-Length: "; temp.append(std::to_string(dataToSend.contentLength)); temp.append("\r\n");
                        addStringToCharList(&rawBuffer, &temp, NULL, -1);
					}
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


					//The block of code bellow sends the buffer in locks of 128 bytes, and , in error case, try to resend blocks
					//{
						sendSize = rawBuffer.size();

						writed = 0;

						while (writed < sendSize)
						{
							for (cBytes = 0; writed + cBytes < sendSize && cBytes <128; cBytes++)
								writeBuffer[cBytes] = rawBuffer.at(cBytes+writed);


							tempWrited = send(client, writeBuffer, cBytes, 0);

							if (tempWrited > 0)
								writed+= tempWrited;
							else
								usleep(10000);
						}
					//}

					rawBuffer.clear();

					//clear data
					delete[] tempIndStrConvert;
					tempIndStrConvert = NULL;

					/*if ((dataToSend.contentBody) && (dataToSend.contentLength > 0))
					{

						delete[] dataToSend.contentBody;
						dataToSend.contentBody = NULL;
					}*/

					receivedData.clear();
					dataToSend.clear();

					state = FINISH_REQUEST;
				break;
				case FINISH_REQUEST:
					state = FINISHED;
				break;
				case ERROR_400_BADREQUEST:
                    //close(client);
					state = FINISHED;
				break;
				case ERROR_500_INTERNALSERVERERROR:
					//close(client);
					state = FINISHED;
				break;
			}

		}

		if (!letSocketOpen)
            close(client);


		delete[] params;
		params = NULL;
		if (tempIndStrConvert)
		{
			delete[] tempIndStrConvert;
			tempIndStrConvert = NULL;
		}

		//pthread_detach(*thTalkWithClient);
		//pthread_exit(0);
	}

	void *ThreadWaitClients(void *thisPointer)
	{
		KWTinyWebServer *self = (KWTinyWebServer*)thisPointer;

		//create an socket to await for connections

		int listener;

		struct sockaddr_in *serv_addr = new sockaddr_in();
		struct sockaddr_in *cli_addr = new sockaddr_in();
		int status;
		socklen_t clientSize;
		pthread_t *thTalkWithClient = NULL;
		vector<int> clients;

		listener = socket(AF_INET, SOCK_STREAM, 0);

		void ** tmp;

		if (listener >= 0)
		{
            int reuse = 1;
			if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)
                self->debug("setsockopt(SO_REUSEADDR) failed");
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

					self->debug("The server is listening and waiting for news connections");
					while (true)
					{
                        int *tempClient = new int;
                        *tempClient = accept(listener, (struct sockaddr *) cli_addr, &clientSize);

						//int client = accept(listener, 0, 0);

						if (*tempClient >= 0)
						{
							tmp = new void*[3];
							tmp[0] = self;
							tmp[1] = (void*)(*tempClient);

							self->__tasks->enqueue([&](void *arguments){
                                HttpProcessThread(arguments);
							}, tmp);


						}
						else{
                            //delete tempClient;
                            usleep(1000);
                        }

					}
				}
				else
					self->debug("Failure to open socket");
			}
			else
				self->debug("Failure to start socket system");
		}
		else
			self->debug("General error opening socket");

		 //n = read(newsockfd,buffer,255);
		 //n = write(newsockfd,"I got your message",18);
	}

	void *WebSocketProcessThread(void * arguments)
	{
        void** params = (void**)arguments;
		KWTinyWebServer *self = (KWTinyWebServer*)params[0];
		int client = *((int*)(params[1]));
		pthread_t *thTalkWithClient = (pthread_t*)(params[2]);
		HttpData *request = (HttpData*)params[3];
		string resource = request->resource;

		params = NULL;

		enum WS_STATES {WS_FINISHED, WS_INTERNAL_SERVER_ERROR, WS_PAYLOAD_NOT_MASKED, WS_READING_PACK_INFO_1, WS_READING_PACK_INFO_2,
                        WS_READ_16BIT_SIZE, WS_READ_64BIT_SIZE, WS_READING_MASK_KEY, WS_READING_PAYLOAD};

		char curr;
		int readCount;
		char readBuffer[1024];

		//packinfo data
        bool fin;
        bool resv3;
        char opcode; //4 bits
        bool masked;
        char packSize7bit;
        int16_t packSize16bit;
        unsigned long long packSize;

        int tempIndex;
        char mask[4];

        char* packPayload;

        vector<char*> payload;
        vector <int> payloadSizes;
        unsigned long long totalPayload = 0;

        WS_STATES state = WS_READING_PACK_INFO_1;

        usleep(2000000);
        self->__observer->OnWebSocketConnect(request, resource);

		while ((state != WS_FINISHED) && (__SocketIsConnected(client)))
		{
			//tempBuffer = new char[2048];

			readCount = recv(client,readBuffer, sizeof(readBuffer), 0);


			if (readCount > 0)
			{
                for (int cont = 0; cont < readCount ; cont++)
                {
                    curr = readBuffer[cont];
                    //READ DATA FROM SOCKET

                    switch (state)
                    {
                        //read the first 2 bytes that contains the information about the incoming pack
                        case WS_READING_PACK_INFO_1:
                            //extract bit finformation
                            //fin indicates that  this pack is the last of the current message (message can be made of many packs)
                            fin = curr & 0b10000000;

                            //the opcode is the type of pack (0x00 is a continuatino pack, 0x01 is a text pack, 0x02 is a binary pack)
                            opcode = curr & 0b00001111;

                            tempIndex = 0;

                            //clear the buffer
                            if (packPayload)
                            {
                                packPayload = NULL;

                            }

                            state = WS_READING_PACK_INFO_2;
                        break;
                        case WS_READING_PACK_INFO_2:
                            //mask indicates if the pack data is masked. Any messages came from client must be masked. If this field has value 0,
                            //the serve must send an error information and closes the socket connection
                            masked = curr & 0b10000000;
                            mask[0] = 0;
                            mask[1] = 0;
                            mask[2] = 0;
                            mask[3] = 0;

                            if (masked == 1)
                            {
                                //read the first byt of packSize. If this value is equals to 126, the server needs to read the next 16 bits to get an
                                //unsigned 16 bit int with the pack sie. If this value is equals to 127, the server must read the next 64 bits to get
                                //an unsigned int64 with the pack size. If the value is less than 126, this is the pack size
                                packSize7bit = curr & 0b01111111;
                                if (packSize7bit < 126 )
                                {
                                    packSize = packSize7bit;
                                    tempIndex = 0;
                                    state  = WS_READING_MASK_KEY;
                                }
                                else if (packSize == 126)
                                    state = WS_READ_16BIT_SIZE;
                                else if (packSize == 127)
                                    state =WS_READ_64BIT_SIZE;
                                else
                                {
                                    //This code never can be executed. If execution reaches this code, the server was erroneus readed  the packSize
                                    //or errnoeus made bit-to-bit process.
                                    state = WS_INTERNAL_SERVER_ERROR;
                                }

                                tempIndex = 0;
                                packSize16bit = 0;

                            }
                            else
                            {
                                self->debug("Error, websocket payload not masked");
                                state = WS_PAYLOAD_NOT_MASKED;

                                //although it goes against the specification of WebSocket, it is possible to interpret unmarked packages from the
                                //client, if this is decided, just ignore the reading of the mask and go straight to the reading of the payload.
                            }
                        break;
                        case WS_READ_16BIT_SIZE:
                            ((char*)&packSize16bit)[tempIndex++] = curr;
                            if (tempIndex ==2)
                            {
                                packSize = packSize16bit;
                                tempIndex = 0;
                                state = WS_READING_MASK_KEY;
                            }
                        break;
                        case WS_READ_64BIT_SIZE:
                            ((char*)&packSize)[tempIndex++] = curr;
                            if (tempIndex == 4)
                            {
                                tempIndex = 0;
                                state = WS_READING_MASK_KEY;
                            }

                        break;
                        case WS_READING_MASK_KEY:
                            mask[tempIndex++] = curr;
                            if (tempIndex == 4)
                            {
                                tempIndex = 0;
                                state = WS_READING_PAYLOAD;
                            }
                        break;
                        case WS_READING_PAYLOAD:
                            if (!packPayload)
                                packPayload = new char[packSize];

                            if (tempIndex < packSize)
                            {
                                packPayload[tempIndex] = curr ^ mask[tempIndex % 4] ;
                                tempIndex++;
                            }

                            //don't use else here, becauze tempIndex is changed on 'if' above and can't enter at code bellow in te last pack byte.

                            if (tempIndex >= packSize)
                            {
                                payload.push_back(packPayload);
                                payloadSizes.push_back(tempIndex);
                                totalPayload += tempIndex;


                                //check if pack is the last of message
                                if (fin)
                                {
                                    tempIndex = 0;
                                    //parepare an object to contaisn the received data

                                    char* fullPayload = new char[totalPayload];


                                    //get all data from all framees of message
                                    for (int cPayload = 0; cPayload < payload.size(); cPayload ++)
                                    {
                                        for (int cData = 0; cData < payloadSizes[cPayload]; cData++)
                                        {
                                            fullPayload[tempIndex++] = payload[cPayload][cData];
                                        }

                                        delete[] payload[cPayload];
                                    }

                                    payload.clear();
                                    payloadSizes.clear();
                                    //send payload to application

                                    self->__observer->OnWebSocketData(request, resource, fullPayload, totalPayload);

                                    //clear payload buffers
                                    delete[] fullPayload;
                                    totalPayload = 0;


                                }
                                packPayload = NULL;

                                tempIndex = 0;


                                state = WS_READING_PACK_INFO_1;

                            }

                        break;
                    }
                }
            }
            else if (readCount == 0)
			{
                state  = WS_FINISHED;
                self->debug("The connection was closed by remote host");

                pthread_detach(*thTalkWithClient);
                pthread_exit(0);
                int ret = 0;
                return (void*)ret;
            }
            else
            {
                //The execution will drop here any time some read error occurrs, including  int ime out cases. In cases of timeout,
                //the "erno" flag will have the value 11 (Resource temporarily unavailable).

                if (errno != 11)
                {
                    string t = "Connection error: " + to_string(errno) + "(" +strerror(errno) + ")";
                    self->debug(t);
                }

                if (errno != 11)
                    state  = WS_FINISHED;
            }
		}

		self->__observer->OnWebSocketDisconnect(request, resource);

		close(client);

		pthread_detach(*thTalkWithClient);
		pthread_exit(0);
	}

    KWTinyWebServer::KWTinyWebServer(int port, WebServerObserver *observer, vector<string> filesLocations, string dataFolder, ThreadPool* tasker)
    {
        if (tasker != NULL)
        {
            cout << "using a custom tasker for httpserver" <<endl;
            this->__tasks = tasker;
        }
        else
            this->__tasks = new ThreadPool(0, 0, "KWBSrvrTsks");

        if (dataFolder == "_AUTO_DEFINE_" || dataFolder == "")
        {
            dataFolder = string(dirname((char*)(get_app_path().c_str()))) + "/data/www_data";

            if (!this->sysLink.directoryExists(dataFolder))
                this->sysLink.createDirectory(dataFolder);
        }
        if (this->__dataFolder.size() > 0 && this->__dataFolder[this->__dataFolder.size()-1] != '/')
            this->__dataFolder += '/';

        this->__dataFolder = dataFolder;

        this->__serverName = "Kiwiisco embeded server, version 0.2.0.0";
        //                                                     | | | |
        //                                                     | | | +----> Bugs fixes
        //                                                     | | +------> Performance and algorithm improvments
        //                                                     | +--------> New features
        //                                                     +----------> Big changes, architecture changes and incompatibilities
        this->__port = port;
        this->__observer = observer;
        this->__filesLocations = filesLocations;
        //ctor
        #ifdef __WIN32__
           WORD versionWanted = MAKEWORD(1, 1);
           WSADATA wsaData;
           WSAStartup(vers ionWanted, &wsaData);
        #endif

        //start internal workers
        for (auto &curr: this->__workers)
            curr->start(this);

        pthread_create(&(this->ThreadAwaitClients), NULL, ThreadWaitClients, this);
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
                string ETag = this->_strUtils.base64_encode((unsigned char*)lastModificationTime.c_str(), lastModificationTime.size());

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

                if (ifNoneMatchHeader == "" || ifNoneMatchHeader != ETag){



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

                        out->contentBody = new char[out->contentLength];
                        sysLink.readFile(filePath, out->contentBody, 0, out->contentLength);
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
                out->contentType = "notFound";
                string t = "the file " + fName + "could not be found ";
                debug(t);
            }
        }
    }

    void KWTinyWebServer::sendWebSocketData(int client, char* data, int size, bool isText)
    {
        // The buffer belllow is enought to contains the header in the all cases (2 bytes of information + max key
        //size, 64 bits)
        char headerBuffer[10 + size] = {0};
        int headerSize = 0;

        //for this method, all packs will be send in one frame. So, determines the fin bit as true
        headerBuffer[0] = headerBuffer[0] | 0b10000000;

        //set opcode (0x01 means that data is a text, 0x02 means that data is binary)
        headerBuffer[0] = isText ? headerBuffer[0] | 0b00000001 : headerBuffer[0] | 0b00000010;

        //al data sent by server must be unmasked so masked indicator bit (first bit of second byte) must be zero.
        //in this case, we just no set this.

        //if data size is less than 126, set this size in the last 7 bits of second byte. If the dataSize is bigget
        //than 126, but less than 65535 (max value of an int16_t, word), set the last 7 bits of second byte with
        //the value 126 and put the size in the next two bytes. If the data size if bigger than 65535, set the last
        //7 bits of second byte with the value 127 and put the data size in the next 64 bits
        if (size < 126)
        {
            uint8_t sizeI8= size;
            headerBuffer[1] = headerBuffer[1] | sizeI8;
            headerSize = 2;
        }
        else if (size < 65536)
        {
            uint16_t sizeI16 = size;
            char* sizeI16Buffer = (char*)&sizeI16;
            headerBuffer[1] = headerBuffer[1] | 126;
            headerBuffer[2] = sizeI16Buffer[1];
            headerBuffer[3] = sizeI16Buffer[0];
            headerSize = 4;
        }
        else
        {
            uint64_t sizeI64 = size;
            char* sizeI64Buffer = (char*)&sizeI64;
            headerBuffer[1] = headerBuffer[1] | 127;
            headerBuffer[2] = sizeI64Buffer[7];
            headerBuffer[3] = sizeI64Buffer[6];
            headerBuffer[4] = sizeI64Buffer[5];
            headerBuffer[5] = sizeI64Buffer[4];
            headerBuffer[6] = sizeI64Buffer[3];
            headerBuffer[7] = sizeI64Buffer[2];
            headerBuffer[8] = sizeI64Buffer[1];
            headerBuffer[9] = sizeI64Buffer[0];
            headerSize = 10;
        }

        for (int c = 0; c < size; c++)
        {
            headerBuffer[c + headerSize] = data[c];
        }

        //sendheader to client
        send(client, headerBuffer, headerSize + size, 0);
        //send data to client
        //send(client, data, size, 0);

    }

    void KWTinyWebServer::sendWebSocketData(HttpData *originalRequest, char* data, int size, bool isText)
    {
        this->sendWebSocketData(originalRequest->client, data, size, isText);
    }

	void KWTinyWebServer::debug(string debugMessage, bool forceFlush)
	{
        cout << "WebServer: " << debugMessage << endl;
        if (forceFlush)
            cout << flush;
	}

	long int KWTinyWebServer::getCurrDayMilisec()
    {

        struct timeval temp;
        gettimeofday(&temp, NULL);

        return temp.tv_sec * 1000 + temp.tv_usec/1000;
    }

    string KWTinyWebServer::get_app_path( )
    {
        char arg1[200];
        char exepath[PATH_MAX + 1] = {0};

        sprintf( arg1, "/proc/%d/exe", getpid() );
        readlink( arg1, exepath, 1024 );
        return string( exepath );
    }

    void KWTinyWebServer::addWorker(IWorker* worker)
    {
        this->__workers.push_back(worker);
        worker->start(this);
    }

}
