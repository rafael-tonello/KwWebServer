#the output file name
binary_name="build/test_server"

#the input files
sources=""
sources="$sources tests/tester.cpp"
sources="$sources sources/HttpData.cpp"
sources="$sources sources/KWTinyWebServer.cpp"
sources="$sources sources/StringUtilFuncs.cpp"
sources="$sources sources/SysLink.cpp"
sources="$sources sources/Workers/CookieParser.cpp"
sources="$sources sources/Workers/HttpSession.cpp"
sources="$sources sources/libs/ThreadPool/ThreadPool.cpp"
sources="$sources sources/libs/TCPServer/sources/TCPServer.cpp"
sources="$sources sources/libs/libs.json_maker/sources/JSON.cpp"

#library paths
#Example of use: libraries="$libraries Lsources/myLibFolder"
libraries="-I./sources"
libraries="$libraries -I./sources/helpers"
libraries="$libraries -I./sources/libs/TCPServer/sources"
libraries="$libraries -I./sources/libs/ThreadPool"
libraries="$libraries -I./sources/Workers"
libraries="$libraries -I./sources/libs/libs.json_maker/sources"

#commands to be runned before compiling process
mkdir build
cp -r sources/copyToBinaryDir/* build/

#the c++ command line
#cpp_cmd="g++ -std=c++17 -ggdb"
cpp_cmd="clang++ -std=c++17 -ggdb"
cpp_aditional_args="-lssl -lcrypto -lpthread"

clear
clear
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' -
sh -c "$cpp_cmd -o $binary_name $libraries $sources $cpp_aditional_args"
