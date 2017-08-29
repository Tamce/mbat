#ifndef _H_PARSER_H_
#define _H_PARSER_H_

#include <string>
#include <sstream>
#include <vector>
#include <stack>
#include <fstream>

namespace tmc {
    using namespace std;
    struct Action
    {
        enum Type {
            none, set, input, echo, tag,
            error = 0xff
        };
        Type type;
        std::string arg1;
        std::string arg2;
        std::string arg3;
        bool hasVar;
        size_t fork;
    };
    
    const char *commands[] = {
        "",
        "set",
        "input",
        "echo",
        ":tag",
    };
    class Parser
    {

        std::vector<Action> actions;
        std::stack<size_t> current;
        void parseLine(const std::string &line, size_t n = 0)
        {
            Action action;
            std::string s;
            static std::stringstream ss;
            ss.clear();
            ss.sync();
            ss.str(line);
            ss >> s;

            // determine type
            action.type = Action::Type::error;
            for (int i = 0; i < sizeof(commands)/sizeof(char *); ++i)
            {
                if (s == commands[i])
                    action.type = Action::Type(i);
            }
            if (action.type == Action::Type::error)
            {
                ss.clear();
                ss.sync();
                ss << "Unknown command `" << s << "` in line " << n;
                throw ss.str();
            }

            // determine args
            switch (action.type)
            {
                case Action::Type::set:
                    getUntil(ss, action.arg1, '=') || syntaxError(n);
                    getUntil(ss, action.arg2, '\n', true, false);
                    break;
                case Action::Type::input:
                    getUntil(ss, action.arg1);
                    action.arg1.size() > 0 || syntaxError(n);
                    break;
                case Action::Type::echo:
                    getUntil(ss, action.arg1, '\n', true, false);
                    break;
                case Action::Type::tag:
                    getUntil(ss, action.arg1);
                    action.arg1.size() > 0 || syntaxError(n);
                    setTag(n, action.arg1);
                    break;
                case Action::Type::none:
                default:
                    break;
            }

            // determine if there is variable in args
            action.hasVar = false;
            for (auto &i : {action.arg1, action.arg2, action.arg3})
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
                    else if (*it == '}' && state == 2) { ++state; break; }
                    else if (state == 1) state = 0;
                    ++it;
                }
                if (state == 3)
                {
                    action.hasVar = true;
                    break;
                }
            }
            actions.emplace_back(action);
        }
        bool syntaxError(size_t n)
        {
            stringstream ss;
            ss << "Syntax error in line " << n;
            throw ss.str();
            return false;
        }
    public:
        Parser(std::ifstream &in)
        {
            if (!in)
                return;
            std::string temp;
            while (in)
            {
                getline(in, temp);
                if (!in) break;
                parseLine(temp);
            }
        }
        void exec()
        {
            exec(0);
        }
        void exec(size_t n)
        {
            while (n < actions.size())
            {
                switch (actions[n].type)
                {
                    case Action::Type::set:
                        cout << "set";
                        break;
                    case Action::Type::input:
                        cout << "input";
                        break;
                    case Action::Type::echo:
                        cout << "echo";
                        break;
                    case Action::Type::tag:
                        cout << "tag";
                        break;
                    default:
                        break;
                }
                cout << endl;
                ++n;
            }
        }
        std::string extendVariable(const std::string &line) {}
        void setVariable(const std::string &key, const std::string &data) {}
        void getVariable(const std::string &key) {}
        void getInput(const std::string &variableKey) {}
        void print(const std::string &str) {}
        void setTag(int n, const std::string &tag) {}
        void jump(const std::string &tag) {}
        void call(const std::string &tag) {}
        void returnCall() {}
        /* Ignore the very first and ending blank characters */
        bool getUntil(std::istream &in, std::string &str, const char c = '\n',
                      bool ltrim = true, bool rtrim = true, bool drop = true)
        {
            bool done = false, reading = false;
            char buff;
            while (!done)
            {
                in.get(buff);
                if (!in) break;
                // trim the very beginning blank characters
                if (!reading && ltrim)
                    if (buff == ' ' || buff == '\t') continue;
                reading = true;

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
            if (rtrim)
            {
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
            }

            // returns if we met the ending character
            return done;
        }

    };
}    
#endif