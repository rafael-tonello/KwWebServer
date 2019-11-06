#include "StringUtils.h"


namespace KWShared{
    StringUtils::StringUtils()
    {
        //ctor
    }

    void StringUtils::split(string str,string sep, vector<string> *result)
    {
         str += "\0";
         /*char* cstr=const_cast<char*>(str.c_str());
         char* current = NULL;
         current=strtok(cstr,sep.c_str());
         while(current!=NULL){
             if (!current)
                 delete[] current;
             result->push_back(current);
             current=strtok(NULL,sep.c_str());
         }

         //delete[] cstr;
         str.clear();
         sep.clear();*/

         //return result;
         string tmp;
         size_t nextIndex = -1;
         while (true)
         {
            nextIndex = str.find(sep);
            if (nextIndex != string::npos)
            {
                result->push_back(str.substr(0, nextIndex));
                if (nextIndex + 1 < str.size())
                    str = str.substr(nextIndex+1);
            }
            else
                break;
         }

         result->push_back(str);
         str.clear();
         sep.clear();
    }

    std::string StringUtils::toUpper(std::string source)
    {
        for (int cont = 0; cont < source.size(); cont++)
            source[cont] = (char)toupper(source[cont]);

        return source;
    }

    std::string StringUtils::toLower(std::string source)
    {
        for (int cont = 0; cont < source.size(); cont++)
            source[cont] = (char)tolower(source[cont]);

        return source;
    }
}
