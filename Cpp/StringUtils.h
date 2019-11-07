#ifndef STRINGUTILS_H
#define STRINGUTILS_H
#include <string>
#include <vector>
#include <algorithm>

namespace KWShared{
    using namespace std;
    class StringUtils
    {
        public:
            StringUtils();

            void split(string str,string sep, vector<string> *result);
            string toUpper(string source);
            string toLower(string source);
            string base64_encode(unsigned char * buf, unsigned int bufLen);
            unsigned char* base64_decode(std::string const& encoded_string);
            string formatDate (time_t dateAndTime);

            void ltrim(std::string &s);
            void rtrim(std::string &s);
            void trim(std::string &s);
    };

    static const std::string base64_chars ="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
}

#endif // STRINGUTILS_H
