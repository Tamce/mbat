// #define DEBUG
#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include "parser.h"
using namespace std;

#define INVALID_ARG 1
#define FILE_ERR 2
#define PARSE_ERR 3

#define COMPILED_HEADER_SIZE 3
#define COMPILED_HEADER "\x66\x66\x66"

#define PWD 0x18

void help()
{
    cout <<
R"(
======= MBAT Parser & Compiler =======
Usage:
    Mbat help                           - Show this message
    Mbat compile scriptFile destFile    - Compile the script to unreadable script file
    Mbat scriptFile                     - Run the script
)";
}

string crypt(const char *file, bool output = true)
{
    fstream f;
    stringstream ss;
    char buff;
    f.open(file, ios::binary | ios::in);
    while (f)
    {
        f.get(buff);
        if (!f) break;
        buff = buff ^ PWD;
        ss.put(buff);
    }
    f.close();
    if (output)
    {
        f.open(file, ios::binary | ios::out | ios::trunc);
        ss.seekg(0);
        while (ss)
        {
            ss.get(buff);
            if (!ss) break;
            f.put(buff);
        }
        return "";
    }
    return ss.str();
}

int main(int argc, char **argv)
{
    ifstream fin;
    ofstream fout;
    tmc::Parser parser;

    if (argc <= 1)
    {
        cerr << "Error: Expected arguments.\n";
        help();
        return INVALID_ARG;
    }

    // Mbat help
    if (argc == 2 && strcmp(argv[1], "help") == 0)
    {
        help();
        return 0;
    }

    // Mbat compile in out
    if (argc == 4 && strcmp(argv[1], "compile") == 0)
    {
        fin.open(argv[2]);
        fout.open(argv[3], ios::binary | ios::trunc);
        if (!fin)
        {
            cerr << "Error opening file " << argv[2] << endl;
            return FILE_ERR;
        }
        if (!fout)
        {
            cerr << "Error opening file " << argv[3] << endl;
            return FILE_ERR;
        }
        try
        {
            parser.parse(fin);
        } catch (string s)
        {
            cerr << "\nError while parsing script file:\n\n" << s << endl;
            return PARSE_ERR;
        }
        fout.write(COMPILED_HEADER, COMPILED_HEADER_SIZE);
        parser.compile(fout);
        fout.close();
        crypt(argv[3]);
        return 0;
    }

    char buff[COMPILED_HEADER_SIZE + 1] = {};
    stringstream ss;
    // Mbat file
    if (argc == 2)
    {
        ss.str(crypt(argv[1], false));
        fin.open(argv[1], ios::binary);
        if (!fin)
        {
            cerr << "Error opening file " << argv[1] << endl;
            return FILE_ERR;
        }

        ss.read(buff, COMPILED_HEADER_SIZE);
        if (strcmp(buff, COMPILED_HEADER) == 0)
        {
            // cryted compiled file
            parser.load(ss);
        } else
        {
            // not compiled file
            fin.close();
            fin.open(argv[1]);
            try
            {
                parser.parse(fin);
            } catch (string s)
            {
                cerr << "\nError while parsing script file:\n\n" << s << endl;
                return PARSE_ERR;
            }
        }
        parser.exec();
        return 0;
    }

    cerr << "\nError: Cannot parse argument(s)!" << endl;
    return INVALID_ARG;
}
