#include "SysLink.h"

namespace KWShared{
    bool SysLink::fileExists(string filename)
	{
		ifstream f(filename.c_str());
		return f.good();
	}

	bool SysLink::deleteFile(string filename)
	{
		//delete(filename.c_str());
		string command = "rm \""+filename+"\"";
		system (command.c_str());

		return SysLink::fileExists(filename);
	}

	bool SysLink::writeFile(string filename, string data)
	{
		ofstream fHandle;
		fHandle.open(filename);
		fHandle << data;
		fHandle.close();
		return true;
	}

	bool SysLink::appendFile(string filename, string data)
	{
		ofstream fHandle;
		if (SysLink::fileExists(filename))
			fHandle.open(filename, ios::out | ios::app);
		else
			fHandle.open(filename);
		fHandle << data;
		fHandle.close();
		return true;
	}

	std::string SysLink::readFile(std:: string filename)
	{
		if (SysLink::fileExists(filename))
		{
			stringstream strStream;
			string result;
			int length;
			ifstream fHandle;
			fHandle.open(filename);
			strStream << fHandle.rdbuf();//read the file
			result = strStream.str();
			fHandle.close();
			return result;
		}
		else
			return "";

	}

	void SysLink::readFile(string filename, char* buffer, unsigned long start, unsigned long count)
	{
		ifstream fHandle;
		fHandle.open(filename);
		fHandle.seekg(start);
		fHandle.read(buffer, count);
		fHandle.close();
	}

	unsigned long SysLink::getFileSize(string filename)
	{
		ifstream fHandle;
		if (SysLink::fileExists(filename))
		{
			/*fHandle.open(filename, ios::binary | ios::ate);
			unsigned long ret = fHandle.tellg();
			fHandle.close();
            cout << "The file size is " << to_string(ret) << endl;
			return ret;*/

			/*struct stat stat_buf;
            int rc = stat(filename.c_str(), &stat_buf);
            return rc == 0 ? stat_buf.st_size : -1;*/

            FILE *p_file = NULL;
            p_file = fopen(filename.c_str(),"rb");
            fseek(p_file,0,SEEK_END);
            int size = ftell(p_file);
            fclose(p_file);
            return size;
		}
		else
			return 0;
	}

	bool SysLink::waitAndLockFile(string filename, int maxTimeout)
	{
		string lockFileName = filename + ".lock";

		while (SysLink::fileExists(lockFileName))
		{
			this->sleep_ms(10);
			if (maxTimeout > 0)
			{
				maxTimeout -= 10;
				if (maxTimeout <= 0)
					break;

			}
		}

		SysLink::writeFile(lockFileName, "locked");
		return true;
	}

	bool SysLink::unlockFile(string filename)
	{
		string lockFileName = filename + ".lock";
		if (SysLink::fileExists(lockFileName))
			SysLink::deleteFile(lockFileName);

		return true;
	}

	bool SysLink::lockFileAndRun(string filename, function<void()> toRun)
	{
		this->waitAndLockFile(filename);
		toRun();
		this->unlockFile(filename);
		

		return true;
	}

	bool SysLink::directoryExists(string directoryName)
	{

		struct stat info;

		if( stat( directoryName.c_str(), &info ) != 0 )
			return false;
		else if( info.st_mode & S_IFDIR )  // S_ISDIR() doesn't exist on my windows
			return true;
		else
			return false;
	}

	bool SysLink::createDirectory(string directoryName)
	{
		string sysCommand = "mkdir -p \""+directoryName+"\"";
		system(sysCommand.c_str());
		return SysLink::directoryExists(directoryName);

	}

	vector<string> SysLink::getFilesFromDirectory(string directoryName, string searchPatern)
	{
		return this->getObjectsFromDirectory(directoryName, searchPatern, "-e \"^-\"");
	}

	vector<string> SysLink::getDirectoriesFromDirectory(string directoryName, string searchPatern)
	{
		return this->getObjectsFromDirectory(directoryName, searchPatern, "-e \"^d\"");
	}

	bool SysLink::deleteDirectory(string directoryName)
	{
		string sysCommand = "rm -rf \""+directoryName+"\"";
		system(sysCommand.c_str());
		return SysLink::directoryExists(directoryName);
	}

	string SysLink::getFileName(string path)
	{
		size_t lastBarIndex = path.rfind("/");
		if (lastBarIndex != string::npos)
		{
			string result = path.substr(lastBarIndex+1, string::npos);
			return result;
		}
		else
			return path;
	}

	string SysLink::getDirectoryName(string path)
	{
		size_t lastBarIndex = path.rfind("/");
		if (lastBarIndex != string::npos)
		{
			string result = path.substr(0, lastBarIndex);
			return result;
		}
		else
			return "";
	}

	void SysLink::sleep_ms(unsigned int ms)
	{
		#ifdef WIN_32
			sleep(ms);
		#endif
		#ifndef WIN_32
			usleep(ms * 1000);
		#endif

	}

	vector<string> SysLink::split(string* text, char sep) {
	  vector<string> tokens;
	  size_t start = 0, end = 0;
	  while ((end = text->find(sep, start)) != string::npos) {
		tokens.push_back(text->substr(start, end - start));
		start = end + 1;
	  }
	  tokens.push_back(text->substr(start));
	  return tokens;
	}


	vector<string> SysLink::getObjectsFromDirectory(string directoryName, string lsFilter, string grepArguments)
	{
		//ls -lh *eita* | grep -e "^-"
		//-rwxrwxrwx. 1 rafinha_tonello rafinha_tonello 220 Jan 13 15:14 receita salame
		vector<string> result;
		int tempIndex;
		string tempString;
		//determine a temporar file name
		string tmpFile = "/tmp/gffd_tmp";

		if (directoryName.back() != '/')
			directoryName += "/";

		//prepre a command to be executed
		string command = "ls --full-time -Gg ''"+directoryName+lsFilter+"'' | grep "+grepArguments+" >\""+tmpFile+"\"";


		string textResult;

		//execute command
		this->waitAndLockFile(tmpFile);
		system(command.c_str());
		textResult = this->readFile(tmpFile);
		this->unlockFile(tmpFile);

		//parse result
		vector<string> lines = this->split(&textResult, '\n');
		if (lines.size() > 1)
		{
			for (int cont =0; cont < lines.size(); cont++)
			{
				tempIndex = lines[cont].find(":");
				if (tempIndex != string::npos)
				{
					tempString = lines[cont].substr(tempIndex+1, string::npos);
					tempIndex = tempString.find(" ");
					if (tempIndex != string::npos)
					{
						tempString = tempString.substr(tempIndex+1, string::npos);
						tempIndex = tempString.find(" ");
						if (tempIndex != string::npos)
						{
							tempString = tempString.substr(tempIndex+1, string::npos);
							result.push_back(tempString);
						}
					}
					tempString.clear();
				}
			}
		}
		lines.clear();
		return result;
	}
}
