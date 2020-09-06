#ifndef SYSLINK_H
#define SYSLINK_H

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


namespace KWShared{
    using namespace std;

    class SysLink
    {
        private:
            vector<string> split(string* text, char sep);
            vector<string> getObjectsFromDirectory(string directoryName, string lsFilter, string grepArguments);
        public:
            //file system
            bool fileExists(string filename);
            bool deleteFile(string filename);
            bool writeFile(string filename, string data);
            bool appendFile(string filename, string data);
            string readFile(string filename);
            void readFile(string filename, char* buffer, unsigned long start, unsigned long count);
            unsigned long getFileSize(string filename);
            bool waitAndLockFile(string filename, int maxTimeout = 1000);
            bool unlockFile(string filename);
            bool directoryExists(string directoryName);
            bool createDirectory(string directoryName);
            vector<string> getFilesFromDirectory(string directoryName, string searchPatern);
            vector<string> getDirectoriesFromDirectory(string directoryName, string searchPatern);
            bool deleteDirectory(string directoryName);
            string getFileName(string path);
            string getDirectoryName(string path);

            //misc
            void sleep_ms(unsigned int ms);
    };
}

#endif // SYSLINK_H
