#include "parser.h"

namespace tmc {
    using namespace std;
    const char *commands[] = {
        "",
        "set",
        "input",
        "echo",
        ":tag",
        "if",
        "goto",
        "echol",
        "calc"
    };

    Action Parser::parseLine(const string &line, size_t n)
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
                // if [arg0] arg1
                getUntil(ss, action.args[0], '[') || syntaxError(n);
                getUntil(ss, action.args[0], ']') || syntaxError(n);
                getUntil(ss, action.args[1]);
                action.args[1].size() > 0 || syntaxError(n);
                break;
            case Action::Type::Goto:
                // goto arg0
                getUntil(ss, action.args[0]);
                action.args[0].size() > 0 || syntaxError(n);
                break;
            case Action::Type::Calc:
                // calc var = 1+123
                getUntil(ss, action.args[0], '=') || syntaxError(n);
                getUntil(ss, action.args[1]);
                action.args[1].size() > 0 || syntaxError(n);
                break;
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

    bool Parser::syntaxError(size_t n)
    {
        stringstream ss;
        ss << "Syntax error in line " << n;
        throw ss.str();
        return false;
    }

    /*
        | len(4) |  tag   | ... |
        | len(4) | action | ... |

        tag:    | lineNumber(4) | len(4) | tagName(len) |
        action: | type(4) | args: len(4), data(len) |
    */
    void Parser::compile(ostream &fout)
    {
        char buff[BUFF_SIZE] = {};
        char *varBuff = nullptr;
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
            varBuff = new char[len + 1];
            memcpy(varBuff, it->first.c_str(), len);
            // len
            fout.write((const char *)&len, sizeof(len));
            // tagName
            fout.write(varBuff, len);
            delete varBuff;
            varBuff = nullptr;
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
                varBuff = new char[len + 1];
                memcpy(varBuff, i.c_str(), len);
                // len
                fout.write((const char *)&len, sizeof(len));
                // content
                fout.write(varBuff, len);
                delete varBuff;
                varBuff = nullptr;
            }
        }
    }

    void Parser::load(istream &fin)
    {
        reset();
        char buff[BUFF_SIZE] = {};
        char *varBuff = nullptr;;
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
            varBuff = new char[len + 1];
            fin.read(varBuff, len);
            tagName = varBuff;
            delete varBuff;
            varBuff = nullptr;

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
                varBuff = new char[len + 1];
                fin.read(varBuff, len);
                i = varBuff;
                delete varBuff;
                varBuff = nullptr;
            }
            actions.emplace_back(action);
        }
    }

    void Parser::reset()
    {
        actions.clear();
        variables.clear();
        tags.clear();
        tags["eof"] = INT_MAX;
    }

    void Parser::parse(istream &in)
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

    void Parser::exec()
    {
        #ifdef DEBUG
        cout << "---begin exec---\n";
        #endif
        for (size_t i = 0; i < actions.size();)
        {
            i = exec(actions[i], i);
        }
    }

    size_t Parser::exec(Action action, size_t n)
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
        static double d1, d2;
        static char c1;
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
                // if [arg0] arg1
                // abc def = efg
                {
                    bool pass = false;
                    string s1, s2;
                    auto pos = action.args[0].find("=");
                    if (pos != string::npos)
                    {
                        s1 = action.args[0].substr(0, pos);
                        // remove last blank characters
                        if (string::npos != s1.find_last_not_of(" \t")) s1 = s1.substr(0, s1.find_last_not_of(" \t") + 1);
                        s2 = action.args[0].substr(pos + 1);
                        // remove the beginning blank chracters
                        if (string::npos != s2.find_first_not_of(" \t")) s2 = s2.substr(s2.find_first_not_of(" \t"));
                        if (s1 == s2) pass = true;
                    } else
                    {
                        ss.clear();
                        ss.str(action.args[0]);
                        ss >> d1 >> c1 >> d2;
                        if (ss.fail() || (c1 != '<' && c1 != '>'))
                        {
                            print("\n\nError evaluating expression `" + action.args[0] + "` in `if` condition.\n");
                            return jump("eof");
                        }
                        if (c1 == '<') pass = d1 < d2;
                        if (c1 == '>') pass = d1 > d2;
                    }
                    if (pass)
                        return exec(parseLine(action.args[1], n), n);
                }
                break;
            case Action::Type::Goto:
                return jump(action.args[0]);
            case Action::Type::Calc:
                ss.clear();
                ss.str(action.args[1]);
                d1 = d2 = c1 = 0;
                ss >> d1 >> c1 >> d2;
                if (ss.fail())
                {
                    print("\n\nRuntime Error: Cannot evaluate expression `" + action.args[1] + "`.\n");
                    return jump("eof");
                } else {
                    ss.clear();
                    ss.str("");
                    switch (c1)
                    {
                        case '+': ss << d1 + d2; break;
                        case '-': ss << d1 - d2; break;
                        case '*': ss << d1 * d2; break;
                        case '/': ss << d1 / d2; break;
                        case '%': ss << (int)d1 % (int)d2; break;
                        default: print("\n\nRuntime Error: Cannot evaluate expression `" + action.args[1] + "`.\n"); return jump("eof");
                    }
                }
                setVariable(action.args[0], ss.str());
                break;
            case Action::Type::Tag:
            default:
                break;
        }
        return n + 1;
    }

    string Parser::extendVariable(string str)
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

    void Parser::setVariable(const string &key, const string &data)
    {
        #ifdef DEBUG
        cout << "set(" << key << ", " << data << ")\n";
        #endif
        variables[key] = data;
    }

    string Parser::getVariable(const string &key)
    {
        if (variables.count(key) > 0)
            return variables[key];
        return "";
    }

    void Parser::getInput(const string &variableKey)
    {
        #ifdef DEBUG
        cout << "input(" << variableKey << ")\n";
        #endif

        string temp;
        getline(cin, temp);
        setVariable(variableKey, temp);
    }

    void Parser::print(const string &str)
    {
        #ifdef DEBUG
        cout << "print(" << str << ")\n";
        #endif

        if (str == "")
            cout << endl;
        else
            cout << str;
    }

    void Parser::setTag(size_t n, const string &tag)
    {
        #ifdef DEBUG
        cout << "setTag(" << n << ", " << tag << ")\n";
        #endif

        tags[tag] = n;
    }

    size_t Parser::jump(const string &tag)
    {
        #ifdef DEBUG
        cout << "jump(" << tag << ")\n";
        #endif
        if (tags.count(tag) > 0)
            return tags.at(tag);
        print("\n\nRuntime Error: Tag `" + tag + "` not found!");
        // Jump to End-Of-File to end the script
        return jump("eof");
    }

    /* Ignore the very first and ending blank characters */
    bool Parser::getUntil(istream &in, string &str, const char c,
                    bool ltrim, bool rtrim, bool drop)
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
}
