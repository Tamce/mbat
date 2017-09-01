// #define DEBUG
#include <iostream>
#include "parser.h"
using namespace std;
int main()
{
    ifstream f("data.txt");
    tmc::Parser p;
    p.parse(f);
    p.exec();
    return 0;
}
