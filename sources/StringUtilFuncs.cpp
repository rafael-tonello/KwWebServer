#include "StringUtilFuncs.h"


vector<string> KWShared::StringUtils::split(string str,string sep)
{
    vector<string> result;
    size_t pos = str.find(sep);
    while (pos != string::npos)
    {
      result.push_back(str.substr(0, pos));
      str = str.substr(pos+1);
      pos = str.find(sep);
    }
    if (str != "")
      result.push_back(str);

    return result;
}

vector<string> KWShared::StringUtils::split(string source, vector<string> possibleSeparators)
{
    vector<string> result;
    
    string currentString = "";
    int i = 0;
    while (i < source.size())
    {
        char c = source[i];

        bool found = false;
        for (auto &d: possibleSeparators)
        {
            if (d.size() > 0 && d[0] == source[i] && source.substr(i, d.size()) == d)
            {
                result.push_back(source.substr(0, i));
                source = source.substr(i+d.size());
                i=0;
                continue;
            }
        }
        i++;
    }
    

    if (source != "")
        result.push_back(source);

    return result;

}

std::string KWShared::StringUtils::toUpper(std::string source)
{
    for (int cont = 0; cont < source.size(); cont++)
        source[cont] = (char)toupper(source[cont]);

    return source;
}

std::string KWShared::StringUtils::toLower(std::string source)
{
    for (int cont = 0; cont < source.size(); cont++)
        source[cont] = (char)tolower(source[cont]);

    return source;
}

typedef unsigned char BYTE;
std::string KWShared::StringUtils::base64_encode(unsigned char * buf, unsigned int bufLen) {
  std::string ret;
  int i = 0;
  int j = 0;
  BYTE char_array_3[3];
  BYTE char_array_4[4];

  while (bufLen--) {
    char_array_3[i++] = *(buf++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while((i++ < 3))
      ret += '=';
  }

  return ret;
}


unsigned char* KWShared::StringUtils::base64_decode(std::string const& encoded_string) {
  int in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  BYTE char_array_4[4], char_array_3[3];
  std::vector<BYTE> ret;

  while (in_len-- && ( encoded_string[in_] != '=') /*&& is_base64(encoded_string[in_])*/) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = base64_chars.find(char_array_4[i]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
          ret.push_back(char_array_3[i]);
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = base64_chars.find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
  }

  //return ret;
  unsigned char* result = new unsigned char[ret.size()];
  for (int c = 0; c < ret.size(); c++)
    result[c] = ret[c];

    return result;
}

string KWShared::StringUtils::formatDate (time_t dateAndTime)
{
    //time_t currentTime;
    //time(&currentTime)
    char buffer[50];
    //Wed, 21 Oct 2015 07:28:00 GMT;
    struct tm* gmt = gmtime(&dateAndTime);

    strftime(buffer, 50, "%a, %d %b %Y %H:%M%S GMT", gmt);
    string ret(buffer);

    return ret;
}

// trim from start (in place)
string KWShared::StringUtils::ltrim(string s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));

    return s;
}

// trim from end (in place)
string KWShared::StringUtils::rtrim(std::string s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());

    return s;
}

// trim from both ends (in place)
string KWShared::StringUtils::trim(std::string s) {
    return ltrim(rtrim(s));
}
