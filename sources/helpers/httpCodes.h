#ifndef __HTTPCODES__H__ 
#define __HTTPCODES__H__ 

#include <string>
#include <map>
 
namespace KWShared{
    using namespace std;
    class HttpCodes { 
    public: 
        static map<int, string> codes;;
        static string get(int code);
    }; 
}
 
#endif 
