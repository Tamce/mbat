// 暂时不把声明与定义分开了
#ifndef _H_PARSER_H_
#define _H_PARSER_H_

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <fstream>
#include <cstdint>
#include <iostream>

#define BUFF_SIZE 512

namespace tmc {
    using namespace std;
    struct Action
    {
        enum Type {
            None, Set, Input, Echo, Tag, If, Goto, Echol, Add
            , error = 0xff
        };
        Type type;
        string args[3];
        bool hasVar;
    };

    extern const char *commands[];

    class Parser
    {
        vector<Action> actions;
        map<string, size_t> tags;
        map<string, string> variables;
        bool syntaxError(size_t n);
        /* Ignore the very first and ending blank characters */
        bool getUntil(istream &in, string &str, const char c = '\n',
                      bool ltrim = true, bool rtrim = true, bool drop = true);
    public:
        Parser() { reset(); }
        void reset();
        void parse(istream &in);
        Action parseLine(const string &line, size_t n = 0);
        void compile(ostream &fout);
        void load(istream &fin);
        size_t exec(Action action, size_t n);
        void exec();
        void setVariable(const string &key, const string &data);
        string getVariable(const string &key);
        string extendVariable(string str);
        void print(const string &str);
        void getInput(const string &variableKey);
        void setTag(size_t n, const string &tag);
        size_t jump(const string &tag);
    };
}    
#endif
