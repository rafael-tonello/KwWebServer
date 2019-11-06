#ifndef COOKIEPARSER_H
#define COOKIEPARSER_H

#include "IWorker.h"
#include "StringUtils.h"

namespace KWShared{
    class CookieParser: public IWorker
    {
        public:
            CookieParser();
            virtual ~CookieParser();

            void load(HttpData* httpData){
                string headerUpper;
                vector<string> temp1;
                string tempKey, tempValue;
                for (auto i = 0; i < httpData->headers.size(); i++)
                {
                    headerUpper = this->strUtils.toUpper((httpData->headers[i][0]));
                    if (headerUpper == "COOKIE")
                    {
                        //void split(string str,string sep, vector<string> *result);
                        this->strUtils.split(httpData->headers[i][1], ";", &temp1);

                        for (auto &currKeyValue: temp1)
                        {
                            //checks for a valid keyValue pair
                            auto equalPos = currKeyValue.find("=");
                            if (equalPos != string::npos)
                            {
                                //gets the key and the value of current cookie
                                tempKey = this->strUtils.toLower(currKeyValue.substr(0, equalPos));
                                tempValue = currKeyValue.substr(equalPos+1);

                                //save cookie in the httpData object

                                if (httpData->cookies.find(tempKey) == httpData->cookies.end())
                                {
                                    httpData->cookies[tempKey] = new HttpCookie();
                                }

                                httpData->cookies[tempKey]->key = tempKey;
                                httpData->cookies[tempKey]->value = tempValue;
                            }
                        }
                    }
                }
            }

            void unload(HttpData* httpData){
                string tempCookieData = "";
                for (auto & curr: httpData->cookies)
                {
                    tempCookieData = curr.second->key + "=" + curr.second->value;

                    if (curr.second->maxAgeSeconds > 0)
                        tempCookieData += "; Max-Age=" + curr.second->maxAgeSeconds;

                    if (curr.second->secure == true)
                        tempCookieData += "; Secure";

                    if (curr.second->httpOnly == true)
                        tempCookieData += "; HttpOnly";


                    httpData->headers.push_back({"Set-Cookie", tempCookieData});
                }
            }

        protected:

        private:
            StringUtils strUtils;
            string formatDate (time_t dateAndTime)
            {
                //time_t currentTime;
                //time(&currentTime)
               char buffer[50];
               //Wed, 21 Oct 2015 07:28:00 GMT;
               struct tm* gmt = gmtime(&dateAndTime);

               strftime(buffer, 50, "%a, %d %b %Y %H:%M%S GMT\0", gmt);
               string ret(buffer);

               return ret;
            }
    };
}
#endif // COOKIEPARSER_H
