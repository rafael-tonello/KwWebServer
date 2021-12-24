#the output file name
binary_name="build/test_server"

#the input files
sources=""
sources="$sources sources/tester.cpp"
sources="$sources sources/HttpData.cpp"
sources="$sources sources/KWTinyWebServer.cpp"
sources="$sources sources/StringUtils.cpp"
sources="$sources sources/SysLink.cpp"
sources="$sources sources/Workers/CookieParser.cpp"
sources="$sources sources/Workers/HttpSession.cpp"
sources="$sources sources/libs/ThreadPool/ThreadPool.cpp"
sources="$sources sources/libs/TCPServer/sources/TCPServer.cpp"
sources="$sources sources/libs/libs.json_maker/cpp/JSON.cpp"

#library paths
#Example of use: libraries="$libraries Lsources/myLibFolder"
libraries="-I./sources"
libraries="$libraries -I./sources/libs/TCPServer/sources"
libraries="$libraries -I./sources/libs/ThreadPool"
libraries="$libraries -I./sources/Workers"
libraries="$libraries -I./sources/libs/libs.json_maker/cpp"

#commands to be runned before compiling process
mkdir build
cp -r sources/copyToBinaryDir/* build/

#the c++ command line
cpp_cmd="g++ -std=c++17 -ggdb -lssl -lcrypto -lpthread"

clear
clear
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' -
sh -c "$cpp_cmd -o $binary_name $libraries $sources"
