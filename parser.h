// 暂时不把声明与定义分开了
#ifndef _H_PARSER_H_
#define _H_PARSER_H_

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <fstream>
#include <cstdint>

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
    
    const char *commands[] = {
        "",
        "set",
        "input",
        "echo",
        ":tag",
        "if",
        "goto",
        "echol",
        "add"
    };

    class Parser
    {

        vector<Action> actions;
        map<string, size_t> tags;
        map<string, string> variables;

        Action parseLine(const string &line, size_t n = 0)
        {
            Action action;

            // empty line and comment
            if (line == "" || line.at(0) == '#')
            {
                action.type = Action::Type::None;
                return action;
            }

            string s;
            static stringstream ss;
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
                case Action::Type::Set:
                    // set arg0 = arg1
                    getUntil(ss, action.args[0], '=') || syntaxError(n);
                    getUntil(ss, action.args[1], '\n', true, false);
                    break;
                case Action::Type::Input:
                    // input arg0
                    getUntil(ss, action.args[0]);
                    action.args[0].size() > 0 || syntaxError(n);
                    break;
                case Action::Type::Echo:
                case Action::Type::Echol:
                    // echo arg0
                    ss.get();       // remove one space
                    getUntil(ss, action.args[0], '\n', false, false);
                    break;
                case Action::Type::Tag:
                    // :tag arg0
                    getUntil(ss, action.args[0]);
                    action.args[0].size() > 0 || syntaxError(n);
                    setTag(n, action.args[0]);
                    break;
                case Action::Type::If:
                    // if [arg0 = arg1] arg2(fork)
                    getUntil(ss, action.args[0], '[') || syntaxError(n);
                    getUntil(ss, action.args[0], '=') || syntaxError(n);
                    getUntil(ss, action.args[1], ']') || syntaxError(n);
                    getUntil(ss, action.args[2]);
                    action.args[2].size() > 0 || syntaxError(n);
                    break;
                case Action::Type::Goto:
                    // goto arg0
                    getUntil(ss, action.args[0]);
                    action.args[0].size() > 0 || syntaxError(n);
                    break;
                case Action::Type::Add:
                    // add var 123
                    ss >> action.args[0];
                    ss >> action.args[1];
                    action.args[1].size() > 0 || syntaxError(n);
                case Action::Type::None:
                default:
                    break;
            }

            // determine if there is variable in args
            action.hasVar = false;
            for (auto &i : action.args)
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
            return action;
        }

        bool syntaxError(size_t n)
        {
            stringstream ss;
            ss << "Syntax error in line " << n;
            throw ss.str();
            return false;
        }
    public:
        Parser()
        {
            reset();
        }

        /*
          | len(4) |  tag   | ... |
          | len(4) | action | ... |

          tag:    | lineNumber(4) | len(4) | tagName(len) |
          action: | type(4) | args: len(4), data(len) |
        */
        void compile(ostream &fout)
        {
            char buff[256] = {};
            int len;
            // tags
            len = tags.size();
            memcpy(buff, &len, sizeof(len));
            // len
            fout.write(buff, sizeof(len));
            for (auto it = tags.begin(); it != tags.end(); ++it)
            {
                // linenNumber
                memcpy(buff, &it->second, sizeof(it->second));
                fout.write(buff, sizeof(it->second));

                len = it->first.size() + 1;
                memcpy(buff, it->first.c_str(), len);
                // len
                fout.write((const char *)&len, sizeof(len));
                // tagName
                fout.write(buff, len);
            }

            // actions
            len = actions.size();
            memcpy(buff, &len, sizeof(len));
            // len
            fout.write(buff, sizeof(len));
            for (auto it = actions.begin(); it != actions.end(); ++it)
            {
                // type
                memcpy(buff, &it->type, sizeof(it->type));
                fout.write(buff, sizeof(it->type));

                // args
                for (auto &i : it->args)
                {
                    len = i.size() + 1;
                    memcpy(buff, i.c_str(), len);
                    // len
                    fout.write((const char *)&len, sizeof(len));
                    // content
                    fout.write(buff, len);
                }
            }
        }

        void load(istream &fin)
        {
            reset();
            char buff[256] = {};
            int len, count;
            string tagName;
            size_t tagN;
            Action action;

            // tags: len
            fin.read(buff, sizeof(count));
            memcpy(&count, buff, sizeof(count));
            for (int i = 0; i < count; ++i)
            {
                // lineNumber
                fin.read(buff, sizeof(tagN));
                memcpy(&tagN, buff, sizeof(tagN));
                // len
                fin.read(buff, sizeof(len));
                memcpy(&len, buff, sizeof(len));
                // tagName
                fin.read(buff, len);
                tagName = buff;

                tags.insert(make_pair(tagName, tagN));
            }

            // actions: len
            fin.read(buff, sizeof(count));
            memcpy(&count, buff, sizeof(count));
            for (int i = 0; i < count; ++i)
            {
                // type
                fin.read(buff, sizeof(action.type));
                memcpy(&action.type, buff, sizeof(action.type));
                // args
                for (auto &i : action.args)
                {
                    fin.read(buff, sizeof(len));
                    memcpy(&len, buff, sizeof(len));
                    fin.read(buff, len);
                    i = buff;
                }
                actions.emplace_back(action);
            }
        }

        void reset()
        {
            actions.clear();
            variables.clear();
            tags.clear();
            tags["eof"] = INT_MAX;
        }

        void parse(istream &in)
        {
            string temp;
            reset();
            while (in)
            {
                getline(in, temp);
                if (!in) break;
                actions.emplace_back(parseLine(temp, actions.size() + 1));
            }
        }

        void exec()
        {
            #ifdef DEBUG
            cout << "---begin exec---\n";
            #endif
            for (size_t i = 0; i < actions.size();)
            {
                i = exec(actions[i], i);
            }
        }
        size_t exec(Action action, size_t n)
        {
            // extend var
            if (action.hasVar)
            {
                for (auto &i : action.args)
                {
                    i = extendVariable(i);
                }
            }

            static stringstream ss;
            static int t1, t2;
            switch (action.type)
            {
                case Action::Type::Set:
                    setVariable(action.args[0], action.args[1]);
                    break;
                case Action::Type::Input:
                    getInput(action.args[0]);
                    break;
                case Action::Type::Echo:
                    print(action.args[0]);
                    break;
                case Action::Type::Echol:
                    print(action.args[0] + "\n");
                    break;
                case Action::Type::If:
                    // cout << "if[" << action.arg1 << "=" << action.arg2 << "]" << action.fork->type << endl;
                    if (action.args[0] == action.args[1])
                        return exec(parseLine(action.args[2], n), n);
                    break;
                case Action::Type::Goto:
                    return jump(action.args[0]);
                case Action::Type::Add:
                    ss.clear();
                    ss.str("");
                    ss << getVariable(action.args[0]) << " " << action.args[1];
                    t1 = t2 = 0;
                    ss >> t1 >> t2;
                    ss.clear();
                    ss.str("");
                    ss << t1 + t2;
                    setVariable(action.args[0], ss.str());
                    break;
                case Action::Type::Tag:
                default:
                    break;
            }
            return n + 1;
        }

        string extendVariable(string str)
        {
            #ifdef DEBUG
            cout << "[extending " << str << "]\n";
            #endif
            // find the very inner variable mark
            string::size_type beg = 0, end = 0;
            while (end != string::npos)
            {
                beg = str.find("${", end);
                end = str.find("${", beg + 1);
            }
            end = str.find("}", beg);

            // no longer variable exist
            if (beg == string::npos || end == string::npos)
                return str;

            #ifdef DEBUG
            cout << "[expanded ${" << str.substr(beg + 2, end - beg - 2) << "} = " << getVariable(str.substr(beg + 2, end - beg - 2)) <<"]\n";
            #endif

            str = str.replace(beg, end - beg + 1,
                getVariable(str.substr(beg + 2, end - beg - 2)));
            return extendVariable(str);
        }

        void setVariable(const string &key, const string &data)
        {
            #ifdef DEBUG
            cout << "set(" << key << ", " << data << ")\n";
            #endif
            variables[key] = data;
        }

        string getVariable(const string &key)
        {
            if (variables.count(key) > 0)
                return variables[key];
            return "";
        }

        void getInput(const string &variableKey)
        {
            #ifdef DEBUG
            cout << "input(" << variableKey << ")\n";
            #endif

            string temp;
            getline(cin, temp);
            setVariable(variableKey, temp);
        }
        void print(const string &str)
        {
            #ifdef DEBUG
            cout << "print(" << str << ")\n";
            #endif

            if (str == "")
                cout << endl;
            else
                cout << str;
        }

        void setTag(size_t n, const string &tag)
        {
            #ifdef DEBUG
            cout << "setTag(" << n << ", " << tag << ")\n";
            #endif

            tags[tag] = n;
        }
        size_t jump(const string &tag)
        {
            #ifdef DEBUG
            cout << "jump(" << tag << ")\n";
            #endif

            return tags.at(tag);
        }

        /* Ignore the very first and ending blank characters */
        bool getUntil(istream &in, string &str, const char c = '\n',
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
