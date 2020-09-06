//compile with g++ -std=c++14 -o saida tester.cpp
#include <iostream>
#include <string>
#include "JSON.h"

using namespace std;
using namespace JsonMaker;

string readFile(string fName);
void test1();
void test2();

int main()
{
    test2();
    return 0;
}

void test1()
{
    JSON *js = new JSON();
    js->setString("mas.em.0", "heheheh");
    js->setString("mas.em.0", "huhu");
    js->setString("mas.em.1", "suahsuahsuhas");

    cout << js->ToJson(true);


    cout << endl << flush;
}

void test2()
{
    string content = readFile("commented.json");

    JSON js(content);

    cout << js.ToJson(true);
    cout << endl << flush;

}

string readFile(string fName)
{
    FILE* f;
    f = fopen(fName.c_str(), "r");
    fseek(f, 0, SEEK_END);
    size_t s = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buffer = new char[s];
    fread(buffer, s, 1, f);
    fclose(f);
    string ret(buffer);
    delete[] buffer;
    return ret;
}