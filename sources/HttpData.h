#ifndef HTTPDATA_H
#define HTTPDATA_H

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <TCPServer.h>


namespace KWShared{
    using namespace TCPServerLib;
    using namespace std;

    //this class represents a http cookie.
    class HttpCookie{
        public:
            HttpCookie(string key = "", string value = ""){this->key = key; this->value = value;}
            HttpCookie(HttpCookie* copyFrom){
                this->copyFrom(copyFrom);
            }

            void copyFrom(HttpCookie* copyFrom)
            {
                this->key = copyFrom->key;
                this->value = copyFrom->value;
                this->maxAgeSeconds = copyFrom->maxAgeSeconds;
                this->secure = copyFrom->secure;
                this->httpOnly= copyFrom->httpOnly;
            }

            ~HttpCookie()
            {
                this->clear();
            }

            void clear()
            {
                this->key = "";
                this->value = "";
                this->secure = false;
                this->httpOnly = false;
                this->maxAgeSeconds = 0;
            }

            string key = "";
            string value = "";
            //time_t expires = 0;
            unsigned int maxAgeSeconds = 0;
            bool secure = false;
            bool httpOnly = false;
    };

    
    class HttpData{
    public:
        HttpData(){}
        HttpData(HttpData* copyFrom){
            this->copyFrom(copyFrom);
        }

        void copyFrom(HttpData* toCopy)
        {
            this->clear();
            this->client = toCopy->client;
            this->resource = toCopy->resource;
            this->method = toCopy->method;
            this->contentType = toCopy->contentType;
            this->contentLength = toCopy->contentLength;
            this->httpStatus = toCopy->httpStatus;
            this->httpMessage = toCopy->httpMessage;

            for (auto &c: toCopy->headers)
                this->headers.push_back({c[0], c[1]});

            for (auto &c: toCopy->cookies)
            {
                this->cookies[c.first] = new HttpCookie(c.second);
            }

            this->contentBody = shared_ptr<char[]>(new char[this->contentLength]);
            for (unsigned int c = 0; c < this->contentLength; c++)
                this->contentBody[c] = toCopy->contentBody[c];
        }

        ~HttpData()
        {
            this->clear();
        }

        void clear()
        {
            resource.clear();
            //this->method.clear();
            //this->contentType.clear();
            this->setContentString("");
            //this->headers.clear();
            //this->httpMessage.clear();


            for (auto &c: this->cookies)
            {
                //c.second->clear();
                delete c.second;
            }
            this->cookies.clear();
        };

        shared_ptr<ClientInfo> client = nullptr;
        string resource = "";
        string method = "";
        string contentType = "";
        string accept = "";
        shared_ptr<char[]> contentBody;
        unsigned int contentLength = 0;
        vector<vector<string> > headers;
        unsigned int httpStatus = 0;
        string httpMessage = "";
        map<string, HttpCookie*> cookies = {};

        void setContentString(string data)
        {
            this->contentBody = shared_ptr<char[]>(new char[data.size()]);
            for (size_t cont = 0; cont < data.size(); cont++)
                this->contentBody[cont] = data[cont];


            this->contentLength = data.size();
        }

        string getContentString()
        {
            string ret = "";
            if (this->contentBody == NULL || this->contentLength == 0)
                return ret;
                
            for (size_t cont = 0; cont < this->contentLength; cont++)
                ret += this->contentBody[cont];

            return ret;
        }
    };
}

#endif // HTTPDATA_H
