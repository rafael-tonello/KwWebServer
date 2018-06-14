#include "StringUtils.h"

#ifndef _MSC_VER
inline
char* strtok_s(char* s, const char *delm, char **context)
{
    return strtok_r(s, delm, context);
}
#endif

namespace Shared
{
    StringUtils::StringUtils()
    {
        //ctor
    }

    StringUtils::~StringUtils()
    {
        //dtor
    }

    void StringUtils::split(string str,string sep, vector<string> *result)
    {
        str += "\0";
        char* cstr=const_cast<char*>(str.c_str());
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
        sep.clear();

        //return result;
    }

    string StringUtils::toUpper(string source)
    {
        for (int cont = 0; cont < source.size(); cont++)
            source[cont] = (char)toupper(source[cont]);

        return source;
    }
}
