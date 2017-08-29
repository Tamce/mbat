#include <iostream>
#include "parser.h"
using namespace std;
int main()
{
    ifstream f("data.txt");
    tmc::Parser p(f);
    p.exec();
    return 0;
}