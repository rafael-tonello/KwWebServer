
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

namespace KWShared{
    using namespace std;

    class SysLink
    {
        private:
            vector<string> split(string* text, char sep);
            vector<string> getObjectsFromDirectory(string directoryName, string lsFilter, string grepArguments);
        public:
            //file system
            bool fileExists(string filename);
            bool deleteFile(string filename);
            bool writeFile(string filename, string data);
            bool appendFile(string filename, string data);
            string readFile(string filename);
            void readFile(string filename, char* buffer, unsigned long start, unsigned long count);
            unsigned long getFileSize(string filename);
            bool waitAndLockFile(string filename, int maxTimeout = 1000);
            bool unlockFile(string filename);
            bool directoryExists(string directoryName);
            bool createDirectory(string directoryName);
            vector<string> getFilesFromDirectory(string directoryName, string searchPatern);
            vector<string> getDirectoriesFromDirectory(string directoryName, string searchPatern);
            bool deleteDirectory(string directoryName);
            string getFileName(string path);
            string getDirectoryName(string path);

            //misc
            void sleep_ms(unsigned int ms);
    };

    class HttpData{
        public:
            int client;
            string resource;
            string method;
            string contentType;
            char* contentBody = NULL;
            unsigned int contentLength = 0;
            vector< vector<string> > headers;
            unsigned int httpStatus;
            string httpMessage;

            void setContentString(string data)
            {
                this->contentBody = new char[data.size()];
                for (int cont = 0; cont < data.size(); cont++)
                    this->contentBody[cont] = data[cont];

                this->contentLength = data.size();
            }
    };

    //typedef void (*OnClientDataSend)(HttpData* in, HttpData* out);

    class WebServerObserver{
        public:
            virtual void OnHttpRequest(HttpData* in, HttpData* out) = 0;
            virtual void OnWebSocketConnect(int client) = 0;
            virtual void OnWebSocketData(int client, char* data, unsigned long long dataSize) = 0;
            virtual void OnWebSocketDisconnect(int client) = 0;
    };


    class KWTinyWebServer
    {
        public:
            KWTinyWebServer(int port, WebServerObserver *observer, vector<string> filesLocations);
            virtual ~KWTinyWebServer();

            void sendWebSocketData(int client, char* data, int size, bool isText);

            void __TryAutoLoadFiles(HttpData* in, HttpData* out);
            int __port;
            WebServerObserver *__observer;
            string __serverName;

        private:
            vector<string> __filesLocations;
            pthread_t ThreadAwaitClients;
            SysLink sysLink;
    };

    class StringUtils
    {
        public:
            StringUtils();

            void split(string str,string sep, vector<string> *result);
            string toUpper(string source);
    };


    using namespace std;


    enum States {AWAIT_HTTP_FIRST_LINE, READING_HEADER, AWAIT_CONTENT, SEND_REQUEST_TO_APP, SEND_RESPONSE, FINISH_REQUEST, FINISHED, ERROR_400_BADREQUEST, ERROR_500_INTERNALSERVERERROR};


    static const std::string base64_chars ="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    typedef unsigned char BYTE;



}
#endif
