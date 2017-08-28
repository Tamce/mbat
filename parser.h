#ifndef _H_PARSER_H_
#define _H_PARSER_H_

#include <string>
#include <sstream>
#include <vector>
#include <stack>
#include <fstream>

namespace tmc {
    typedef struct Line
    {
        int n;
        string content;
    } Tag, Error;

    struct Script
    {
        std::vector<Line> content;
        std::vector<Tag> tags;
    };

    class Parser
    {
        Script script;
        std::stack<int> current;
        Error e;
        /* From script.content parse script.tags and Check syntax*/
        void parse()
        {
            std::stringstream ss;
            std::string s1, s2;
            for (auto &line : script.content)
            {
                ss.clear();
                s1 = s2 = "";
                line.content = extendVariable(line.content);
                ss.str(line.content);
                ss >> s1;
                if (s1 == SET)
                {
                    s1 = "";
                    getUntil(ss, s1, '=');
                    getUntil(ss, s2);
                    setVariable(s1, s2);
                } else if (s1 == INPUT)
                {
                    getUntil(ss, s2);
                    getInput(s2);
                } else if (s1 == ECHO)
                {
                    getUntil(ss, s2);
                    print(s2);
                } else if (s1 == TAG)
                {
                    getUntil(ss, s2);
                    setTag(line.n, s2);
                } // todo
            }
        }
    public:
        Parser(std::ifstream &in): e({-1, ""}), current({0})
        {
            if (!in)
                return;
            std::string temp;
            while (in)
            {
                getline(in, temp);
                script.content.push_back(temp);
            }
            parse();
        }
        void exec();
        void exec(const std::string &line);
        string extendVariable(const string &line);
        void setVariable(const string &key, const std::string &data);
        void getVariable(const std::string &key);
        void getInput(const std::string &variableKey);
        void print(const std::string &str);
        void setTag(int n, const std::string &tag);
        void jump(const std::string &tag);
        void call(const std::string &tag);
        void returnCall();
        /* Ignore the very first and ending blank characters */
        bool getUntil(std::istream &in, std::string &str, const char c = '\n', bool drop = true)
        {
            bool done = false;
            char buff;
            while (in && !done)
            {
                in >> buff;
                if (buff == c)
                {
                    if (!drop)
                        in.putback(buff);
                    done = true;
                } else {
                    str.push_back(buff);
                }
            }

            // trim the ending blank characters
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
            return done;
        }

        static const char *SET = "set";
        static const char *INPUT = "input";
        static const char *ECHO = "echo";
        static const char *TAG = ":tag";
    };
}    
#endif