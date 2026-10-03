#pragma once
#include <unordered_map>

struct shader_token_t
{
    std::string text;
};

class shader_rewrite_t
{
    struct expression_t
    {
        std::string text;
        int type;
        expression_t(std::string value = "", int kind = 0) : text(std::move(value)), type(kind) {}
    };
    std::vector<shader_token_t> tokens;
    std::unordered_map<std::string, int> types;
    std::unordered_map<std::string, int> samplers;
    size_t position = 0;

    static int Type(const std::string& name)
    {
        if(name == "float") return 2;
        if(name.size() == 4 && (name.compare(0, 3, "vec") == 0 || name.compare(0, 3, "mat") == 0) && name[3] >= '2' && name[3] <= '4') return 2;
        if(name.size() == 6 && name.compare(0, 3, "mat") == 0 && name[3] >= '2' && name[3] <= '4' && name[4] == 'x' && name[5] >= '2' && name[5] <= '4') return 2;
        if(name == "bool") return 3;
        if(name.size() == 5 && (name.compare(0, 4, "ivec") == 0 || name.compare(0, 4, "uvec") == 0 || name.compare(0, 4, "bvec") == 0) && name[4] >= '2' && name[4] <= '4') return 3;
        if(name == "int" || name == "uint") return 1;
        return 0;
    }
    static int Priority(const std::string& op)
    {
        if(op == "*" || op == "/" || op == "%") return 10;
        if(op == "+" || op == "-") return 9;
        if(op == "<<" || op == ">>") return 8;
        if(op == "<" || op == ">" || op == "<=" || op == ">=") return 7;
        if(op == "==" || op == "!=") return 6;
        if(op == "&") return 5;
        if(op == "^") return 4;
        if(op == "|") return 3;
        if(op == "&&") return 2;
        if(op == "||" || op == "^^") return 1;
        return 0;
    }
    std::string Peek() const { return position < tokens.size() ? tokens[position].text : ""; }
    expression_t Primary()
    {
        if(position == tokens.size()) return {};
        std::string name = tokens[position++].text;
        expression_t value(name, types.count(name) ? types[name] : 0);
        if(name == "+" || name == "-" || name == "!" || name == "~")
        {
            value = Primary();
            value.text = name + " " + value.text;
            if(name == "!") value.type = 0;
            return value;
        }
        if(!name.empty() && (std::isdigit((unsigned char)name[0]) || name[0] == '.'))
            value.type = name.compare(0, 2, "0x") != 0 && name.find_first_of(".eEfF") != std::string::npos ? 2 : 1;
        if(name == "(")
        {
            value = Expression(1);
            if(Peek() == ")") { ++position; value.text = "(" + value.text + ")"; }
            else value.text = "(" + value.text;
        }
        else if(Peek() == "(")
        {
            ++position;
            std::vector<expression_t> args;
            while(position < tokens.size() && Peek() != ")")
            {
                size_t previous = position;
                args.push_back(Expression(1));
                if(position == previous) break;
                if(Peek() != ",") break;
                ++position;
            }
            bool closed = Peek() == ")";
            if(closed) ++position;
            int sampler = 0;
            if(!args.empty())
            {
                std::string id = args[0].text.substr(0, args[0].text.find_first_of(" ["));
                if(samplers.count(id)) sampler = samplers[id];
            }
            std::string function = name;
            if(sampler && args.size() >= 2)
            {
                if(sampler == 1)
                {
                    if(name == "texture" || name == "textureLod" || name == "textureGrad")
                    {
                        args[1].text = "vec2(" + args[1].text + ",0.5)";
                        if(name == "textureGrad" && args.size() >= 4)
                            for(size_t i = 2; i < 4; ++i) args[i].text = "vec2(" + args[i].text + ",0.0)";
                    }
                    else if(name == "texelFetch") args[1].text = "ivec2(" + args[1].text + ",0)";
                }
                else if(name == "texture")
                    args[1].text = "(" + args[1].text + "/vec2(textureSize(" + args[0].text + ",0)))";
            }
            if(sampler == 2 && (name == "textureSize" || name == "texelFetch")) args.push_back(expression_t("0", 1));
            value.text = function + "(";
            for(size_t i = 0; i < args.size(); ++i) value.text += (i ? "," : "") + args[i].text;
            if(closed) value.text += ")";
            if(sampler == 1 && name == "textureSize") value.text += ".x";
            value.type = Type(name);
            if(!value.type && types.count(name)) value.type = types[name];
        }
        while(position < tokens.size())
        {
            if(Peek() == "." && position + 1 < tokens.size())
            {
                ++position;
                value.text += "." + tokens[position++].text;
            }
            else if(Peek() == "[")
            {
                ++position;
                expression_t index = Expression(1);
                value.text += "[" + index.text;
                if(Peek() == "]") { ++position; value.text += "]"; }
            }
            else break;
        }
        return value;
    }
    expression_t Expression(int minimum)
    {
        expression_t left = Primary();
        while(position < tokens.size())
        {
            std::string op = Peek();
            int priority = Priority(op);
            if(priority < minimum) break;
            ++position;
            expression_t right = Expression(priority + 1);
            if((priority >= 9 || priority == 6 || priority == 7) && op != "%")
            {
                if(left.type == 2 && right.type == 1) right.text = "float(" + right.text + ")";
                if(left.type == 1 && right.type == 2) left.text = "float(" + left.text + ")";
            }
            left.text += " " + op + " " + right.text;
            left.type = priority >= 9 ? (left.type && right.type ? std::max(left.type, right.type) : 0) : 0;
        }
        return left;
    }
public:
    explicit shader_rewrite_t(const std::string& source)
    {
        for(size_t i = 0; i < source.size();)
        {
            if(std::isspace((unsigned char)source[i])) { ++i; continue; }
            size_t start = i++;
            if(source.compare(start, 2, "//") == 0)
            {
                i = source.find('\n', i);
                if(i == std::string::npos) i = source.size();
                continue;
            }
            if(source.compare(start, 2, "/*") == 0)
            {
                i = source.find("*/", i);
                i = i == std::string::npos ? source.size() : i + 2;
                continue;
            }
            if(source[start] == '#')
            {
                i = source.find('\n', i);
                while(i != std::string::npos && i > start && source[i-1] == '\\') i = source.find('\n', i + 1);
                if(i == std::string::npos) i = source.size();
                tokens.push_back({"\n" + source.substr(start, i - start) + "\n"});
                continue;
            }
            if(std::isalpha((unsigned char)source[start]) || source[start] == '_')
                while(i < source.size() && (std::isalnum((unsigned char)source[i]) || source[i] == '_')) ++i;
            else if(std::isdigit((unsigned char)source[start]) || (source[start] == '.' && i < source.size() && std::isdigit((unsigned char)source[i])))
            {
                while(i < source.size())
                {
                    char c = source[i];
                    if(std::isalnum((unsigned char)c) || c == '.') ++i;
                    else if((c == '+' || c == '-') && i > start && (source[i-1] == 'e' || source[i-1] == 'E')) ++i;
                    else break;
                }
            }
            else if(i < source.size())
            {
                if(source.compare(start, 3, "<<=") == 0 || source.compare(start, 3, ">>=") == 0)
                {
                    i = start + 3;
                    tokens.push_back({source.substr(start, 3)});
                    continue;
                }
                std::string pair = source.substr(start, 2);
                if(pair == "++" || pair == "--" || pair == "<<" || pair == ">>" || pair == "==" || pair == "!=" ||
                   pair == "<=" || pair == ">=" || pair == "&&" || pair == "||" || pair == "^^" ||
                   pair == "+=" || pair == "-=" || pair == "*=" || pair == "/=" || pair == "%=" || pair == "&=" || pair == "|=" || pair == "^=") ++i;
            }
            tokens.push_back({source.substr(start, i - start)});
        }
        types["gl_Position"] = 2;
        types["gl_VertexID"] = 1;
        types["gl_InstanceID"] = 1;
        for(size_t i = 0; i + 1 < tokens.size(); ++i)
        {
            int type = Type(tokens[i].text);
            if(type && tokens[i+1].text != "(")
            {
                auto found = types.find(tokens[i+1].text);
                if(found == types.end()) types[tokens[i+1].text] = type;
                else if(found->second != type) found->second = 0;
            }
            if(tokens[i].text == "sampler1D" || tokens[i].text == "sampler2DRect")
            {
                samplers[tokens[i+1].text] = tokens[i].text == "sampler1D" ? 1 : 2;
                tokens[i].text = "sampler2D";
            }
        }
    }
    std::string Run()
    {
        std::string output;
        while(position < tokens.size())
        {
            size_t previous = position;
            output += Expression(1).text + " ";
            if(position == previous) output += tokens[position++].text + " ";
        }
        return output;
    }

    bool HasOutputIndex() const
    {
        for(size_t i = 0; i + 1 < tokens.size(); ++i)
        {
            if(tokens[i].text != "layout" || tokens[i+1].text != "(") continue;
            int depth = 1;
            for(size_t j = i + 2; j + 1 < tokens.size() && depth; ++j)
            {
                if(tokens[j].text == "(") ++depth;
                else if(tokens[j].text == ")") --depth;
                else if(depth == 1 && tokens[j].text == "index" && tokens[j+1].text == "=") return true;
            }
        }
        return false;
    }
};
