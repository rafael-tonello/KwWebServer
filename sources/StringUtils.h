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

            static void split(string str,string sep, vector<string> *result);
            static string toUpper(string source);
            static string toLower(string source);
            static string base64_encode(unsigned char * buf, unsigned int bufLen);
            static unsigned char* base64_decode(std::string const& encoded_string);
            static string formatDate (time_t dateAndTime);

            static string ltrim(string s);
            static string rtrim(string s);
            static string trim(string s);
    };

    static const std::string base64_chars ="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
}

#endif // STRINGUTILS_H
