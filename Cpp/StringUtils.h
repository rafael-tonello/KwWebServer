#ifndef STRINGUTILS_H
#define STRINGUTILS_H
#include <string>
#include <vector>

namespace KWShared{
    using namespace std;
    class StringUtils
    {
        public:
            StringUtils();

            void split(string str,string sep, vector<string> *result);
            string toUpper(string source);
            string toLower(string source);
    };
}

#endif // STRINGUTILS_H
