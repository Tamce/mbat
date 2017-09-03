#include <string>
#include <sstream>
#include <iostream>
using namespace std;

int main()
{
    string s2= "   abc def = abc def    ";
    if (string::npos != s2.find_first_not_of(" \t")) s2 = s2.substr(s2.find_first_not_of(" \t"));
    cout << s2 << "|";
    return 0;
}
