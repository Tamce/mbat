#include <string>
#include <sstream>
#include <iostream>
using namespace std;

int main()
{
    string str1 = "abc${d}f", str2 = "fk$j{jfkd}", str3 = "dj${jfkd";
    for (auto &i : {str1, str2, str3})
    {
        auto it = i.begin();
        // 0: waiting for '$'
        // 1: waiting for '{'
        // 2: waiting for '}'
        // 3: variable found
        int state = 0;
        while (it != i.end())
        {
            if (*it == '$' && state == 0) ++state;
            else if (*it == '{' && state == 1) ++state;
            else if (*it == '}' && state == 2) {++state; break;}
            else if (state == 1) state = 0;
            ++it;
        }
        if (state == 3) cout << i << endl;
    }
    return 0;
}