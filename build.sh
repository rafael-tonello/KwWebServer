#the output file name
binary_name="bin/json_tests"

#the input files
sources=""
sources="$sources sources/JSON.cpp"
sources="$sources sources/tester.cpp"

#library paths
#Example of use: libraries="$libraries Lsources/myLibFolder"
libraries=""

#commands to be runned before compiling process
mkdir bin
cp -r sources/copyToBinaryDir/* bin/

#the c++ command line
cpp_cmd="g++ -std=c++17 -ggdb"

clear
clear
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' -
sh -c "$cpp_cmd -o $binary_name $sources $libraries"
