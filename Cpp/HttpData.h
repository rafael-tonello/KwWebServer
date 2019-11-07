#ifndef HTTPDATA_H
#define HTTPDATA_H

#include <string>
#include <vector>
#include <map>
#include <iostream>


namespace KWShared{
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
            ~HttpData()
            {
                this->clear();
            }

            void clear()
            {
                resource.clear();
                this->method.clear();
                this->contentType.clear();
                this->setContentString("");
                this->headers.clear();
                this->httpMessage.clear();

                for (auto &c: this->cookies)
                {
                    delete c.second;
                }
                this->cookies.clear();
            };

            int client;
            string resource;
            string method;
            string contentType;
            char* contentBody = NULL;
            unsigned int contentLength = 0;
            vector< vector<string> > headers;
            unsigned int httpStatus;
            string httpMessage;
            map<string, HttpCookie*> cookies;

            void setContentString(string data)
            {
                if (this->contentBody != NULL)
                    delete[] this->contentBody;

                this->contentBody = new char[data.size()];
                for (int cont = 0; cont < data.size(); cont++)
                    this->contentBody[cont] = data[cont];

                this->contentLength = data.size();
            }
    };
}

#endif // HTTPDATA_H
