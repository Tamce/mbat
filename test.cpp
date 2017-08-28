#include <string>
#include <iostream>
using namespace std;
int main()
{
    string str = "  a  ";
    auto i = str.end();
    while (i != str.begin())
    {
        --i;
        if (*i == ' ' || *i == '\t')
            continue;
        ++i;
        break;
    }
    str.erase(i, str.end());
    cout << str << "|";
    return 0;
}