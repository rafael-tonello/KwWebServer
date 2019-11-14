//This cla sses uses the library JsonMaker: https://github.com/SerraFullStack/libs.json_maker
#ifndef HTTPSESSION_H
#define HTTPSESSION_H

#include "../../../libs.json_maker/cpp/JSON.h"
#include "../KWTinyWebServer.h"
#include "../IWorker.h"


namespace KWShared{
    using namespace JsonMaker;
    class HttpSession: public IWorker
    {
        protected:
            map<string, JSON*> currentOpenedSessions;

        private:
            unsigned int uniqueCount  = 0;
            StringUtils strUtils;
            string getFileNameFromSsid(string ssid)
            {
                string nId = "";
                string validChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
                for (auto &c: ssid)
                {
                    if (validChars.find(c) != string::npos)
                        nId += c;
                    else
                        nId += "_";
                }
                return this->dataFolder + nId +".json";
            }

            string getUniqueId(){
                //prepare a unique data
                string hasgUniqueData = "";
                hasgUniqueData += uniqueCount;
                hasgUniqueData += strUtils.formatDate(time(NULL));

                //calculates the SHA1 hash of the prepared data
                unsigned char sha1result[SHA_DIGEST_LENGTH];
                SHA1((unsigned char*)hasgUniqueData.c_str(), hasgUniqueData.size(), sha1result);
                string sha1Result = this->strUtils.base64_encode(sha1result, SHA_DIGEST_LENGTH);

                sha1Result+= uniqueCount++;

                //return the SHA1 as result
                return sha1Result;
            }

            SysLink sysLink;
            string dataFolder;

        public:
            void start(void* webserver){
                SysLink sysLink;

                this->dataFolder = ((KWTinyWebServer*)webserver)->__dataFolder + "/sessions/";
                if (!sysLink.directoryExists(this->dataFolder))
                    sysLink.createDirectory(this->dataFolder);
            };

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
                httpData->cookies["ssid"]->secure = false;


            }

            void unload(HttpData* httpData){
                //find session object
                JSON* session = this->getSessionData(httpData);
                if (session != NULL)
                {

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
            }

            JSON* getSessionData(HttpData* httpData)
            {
                //for securty, checks if exists a openes session corresponding to the 'ssid' cookie
                if (httpData->cookies.find("ssid") != httpData->cookies.end())
                {
                    string ssid = httpData->cookies["ssid"]->value;

                    //checks if session is already loaded from file
                    if (this->currentOpenedSessions.find(ssid) == this->currentOpenedSessions.end())
                    {
                        //determine the fiename of file with session data
                        string sessionFileName = getFileNameFromSsid(ssid);

                        //create a new json to receive the data of the current session
                        this->currentOpenedSessions[ssid] = new JSON();
                        if (this->sysLink.fileExists(sessionFileName))
                            this->currentOpenedSessions[ssid]->parseJson(this->sysLink.readFile(sessionFileName));
                    }

                    return this->currentOpenedSessions[ssid];
                }
                else
                    return NULL;
            }
    };
}

#endif // HTTPSESSION_H
