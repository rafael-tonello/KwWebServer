#ifndef STRINGUTILS_H
#define STRINGUTILS_H
#include <string>
#include <strings.h>
#include <vector>
#include <cstring>

using namespace std;


namespace Shared
{
    class StringUtils
    {
        public:
            StringUtils();
            virtual ~StringUtils();

            void split(string str,string sep, vector<string> *result);
            string toUpper(string source);

        protected:

        private:
    };
}
#endif // STRINGUTILS_H
