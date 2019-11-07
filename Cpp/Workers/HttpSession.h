//This cla sses uses the library JsonMaker: https://github.com/SerraFullStack/libs.json_maker
#ifndef HTTPSESSION_H
#define HTTPSESSION_H

#include "../../libs.json_maker/cpp/JSON.h"
#include "KWTinyWebServer.h"
#include "IWorker.h"


namespace KWShared{
    using namespace JsonMaker;
    class HttpSession: IWorker
    {
        protected:
            map<string, JSON*> currentOpenedSessions;

        private:
            unsigned int uniqueCount  = 0;
            StringUtils strUtils;
            string getFileNameFromSsid(string ssid)
            {

            }

            string getUniqueId(){
                //prepare a unique data
                string hasgUniqueData = "";
                hasgUniqueData += uniqueCount++;
                hasgUniqueData += strUtils.formatDate(time(NULL));

                //calculates the SHA1 hash of the prepared data
                unsigned char sha1result[SHA_DIGEST_LENGTH];
                SHA1((unsigned char*)hasgUniqueData.c_str(), hasgUniqueData.size(), sha1result);
                string sha1Result = this->strUtils.base64_encode(sha1result, SHA_DIGEST_LENGTH);

                //return the SHA1 as result
                return sha1Result;
            }

            SysLink sysLink;

        public:
            HttpSession(string sessionsFolder);
            void load(HttpData* httpData){
                //get the session cookie
                string ssid = "";
                if (httpData->cookies.find("ssid") == httpData->cookies.end() || httpData->cookies["ssid"] == NULL)
                {
                    //create a cookie with a new ssid
                    ssid = this->getUniqueId();

                    httpData->cookies["ssid"] = new HttpCookie("ssid", ssid);
                }

                ssid = httpData->cookies["ssid"]->value;

                //ensures 'ssid' cookie security
                httpData->cookies["ssid"]->secure = true;

                //determine the fiename of file with session data
                string sessionFileName = getFileNameFromSsid(ssid);

                //create a new json to receive the data of the current session
                this->currentOpenedSessions[ssid] = new JSON();
                if (this->sysLink.fileExists(sessionFileName))
                    this->currentOpenedSessions[ssid]->parseJson(this->sysLink.readFile(sessionFileName));
            }

            void unload(JSON* session, HttpData* httpData){
                //save session data to a file

                //get the JSON data
                string data = session->ToJson();

                //get the ssid
                string ssid = httpData->cookies["ssid"]->value;

                string sessionFileName = getFileNameFromSsid(ssid);

                //write data to file
                this->sysLink.writeFile(sessionFileName, data);

                //remvoe JSON object from 'currentOpenedSessions'
                session->clear();
                currentOpenedSessions[ssid] = NULL;
                currentOpenedSessions.erase(ssid);
            }

            JSON* getSessionData(HttpData* httpData)
            {
                //checks if 'ssid' cookie exists
                if (httpData->cookies.find("ssid") != httpData->cookies.end())
                {
                    //for securty, checks if exists a openes session corresponding to the 'ssid' cookie
                    if (this->currentOpenedSessions.find(httpData->cookies["ssid"]->value) != this->currentOpenedSessions.end())
                        return this->currentOpenedSessions[httpData->cookies["ssid"]->value];
                    else
                        return NULL;
                }
                else
                    return NULL;
            }
    };
}

#endif // HTTPSESSION_H
