#ifndef COOKIEPARSER_H
#define COOKIEPARSER_H

#include <IWorker.h>
#include <StringUtils.h>
#include <iostream>
namespace KWShared{
    using namespace std;
    class CookieParser: public IWorker
    {
        public:

            void start(void* webserver){};
            void load(HttpData* httpData){
                /*this worker scrolls throught the headers of the httpData object and looks for
                cookies information and uses this information to populate the 'cookies' property of the'
                httpData*/

                string headerUpper;
                vector<string> temp1;
                string tempKey, tempValue;
                //pass over all headers
                for (auto i = 0; i < httpData->headers.size(); i++)
                {
                    headerUpper = this->strUtils.toUpper((httpData->headers[i][0]));
                    //checks if the current hreader is a cookie information
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
                                tempValue = this->strUtils.toLower(currKeyValue.substr(equalPos+1));

                                //trim key and value
                                tempKey = this->strUtils.trim(tempKey);
                                tempValue = this->strUtils.trim(tempValue);

                                //save cookie in the httpData object
                                if (!httpData->cookies.count(tempKey))
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
                        tempCookieData += "; Max-Age=" + std::to_string(curr.second->maxAgeSeconds);

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

    };
}
#endif // COOKIEPARSER_H
