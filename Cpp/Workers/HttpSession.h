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
            StringUtils strUtils;
            string getFileNameFromSsid(string ssid)
            {

            }

            SysLink sysLink;

        public:
            HttpSession(string sessionsFolder);
            JSON* load(HttpData* httpData){
                //get the session cookie
                string ssid = "";
                if (httpData->cookies.find("ssid") == httpData->cookies.end())
                {
                    //create a cookie with a new ssid
                }

                ssid = httpData->cookies["ssid"];

                //determine the fiename of file with session data
                string sessionFileName = getFileName();

                //create a new json to receive the data of the current session
                this->currentOpenedSessions[ssid]
                if (this->sysLink.fileExists(sessionFileName))





            };

            void unload(JSON* session, HttpData* httpData){

            };
    };
}

#endif // HTTPSESSION_H
