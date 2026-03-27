#ifndef KISEKI_HPP
#define KISEKI_HPP

#include <algorithm>
#include <cctype>
#include <map>
#include <memory>
#include <stack>
#include <string>
#include <unordered_set>
#include <vector>

namespace Kiseki {

struct Node : public std::enable_shared_from_this<Node> {
    std::string tag;
    std::string raw_text;
    std::map<std::string, std::string> attributes;
    std::vector<std::shared_ptr<Node>> children;

    std::string text() const {
        std::string result = raw_text;
        for (const auto& child : children) {
            result += child->text();
        }
        return result;
    }

    std::string attr(const std::string& key) const {
        auto it = attributes.find(key);
        return it == attributes.end() ? "" : it->second;
    }

    std::vector<std::shared_ptr<Node>> find(const std::string& selector) {
        std::vector<std::shared_ptr<Node>> results;
        if (selector.empty()) {
            return results;
        }

        std::string type = "tag";
        std::string query = selector;
        if (selector[0] == '.') {
            type = "class";
            query = selector.substr(1);
        } else if (selector[0] == '#') {
            type = "id";
            query = selector.substr(1);
        }

        for (auto& child : children) {
            child->searchNodes(type, query, results);
        }
        return results;
    }

private:
    static std::vector<std::string> splitClasses(const std::string& classes) {
        std::vector<std::string> out;
        std::string token;
        for (char c : classes) {
            if (std::isspace(static_cast<unsigned char>(c))) {
                if (!token.empty()) {
                    out.push_back(token);
                    token.clear();
                }
            } else {
                token.push_back(c);
            }
        }
        if (!token.empty()) {
            out.push_back(token);
        }
        return out;
    }

    void searchNodes(const std::string& type, const std::string& query,
                     std::vector<std::shared_ptr<Node>>& results) {
        bool match = false;
        if (type == "tag" && tag == query) {
            match = true;
        } else if (type == "id" && attr("id") == query) {
            match = true;
        } else if (type == "class") {
            for (const auto& className : splitClasses(attr("class"))) {
                if (className == query) {
                    match = true;
                    break;
                }
            }
        }

        if (match) {
            results.push_back(shared_from_this());
        }

        for (auto& child : children) {
            child->searchNodes(type, query, results);
        }
    }
};

class Parser {
private:
    static bool isSelfClosingTag(const std::string& tagName) {
        static const std::unordered_set<std::string> selfClosing = {
            "img", "br", "hr", "input", "meta", "link", "source", "area", "base", "col", "embed", "param", "track", "wbr"};
        return selfClosing.count(tagName) > 0;
    }

    static std::string trim(const std::string& s) {
        const auto begin = s.find_first_not_of(" \n\r\t");
        if (begin == std::string::npos) {
            return "";
        }
        const auto end = s.find_last_not_of(" \n\r\t");
        return s.substr(begin, end - begin + 1);
    }

    static void parseTagContent(const std::string& raw, const std::shared_ptr<Node>& node) {
        std::string content = trim(raw);
        if (!content.empty() && content.back() == '/') {
            content.pop_back();
            content = trim(content);
        }

        size_t i = 0;
        while (i < content.size() && !std::isspace(static_cast<unsigned char>(content[i]))) {
            node->tag.push_back(content[i++]);
        }

        while (i < content.size()) {
            while (i < content.size() && std::isspace(static_cast<unsigned char>(content[i]))) {
                ++i;
            }
            if (i >= content.size()) {
                break;
            }

            std::string key;
            while (i < content.size() && content[i] != '=' &&
                   !std::isspace(static_cast<unsigned char>(content[i]))) {
                key.push_back(content[i++]);
            }

            while (i < content.size() && std::isspace(static_cast<unsigned char>(content[i]))) {
                ++i;
            }

            std::string value;
            if (i < content.size() && content[i] == '=') {
                ++i;
                while (i < content.size() && std::isspace(static_cast<unsigned char>(content[i]))) {
                    ++i;
                }

                if (i < content.size() && (content[i] == '"' || content[i] == '\'')) {
                    const char quote = content[i++];
                    while (i < content.size() && content[i] != quote) {
                        value.push_back(content[i++]);
                    }
                    if (i < content.size() && content[i] == quote) {
                        ++i;
                    }
                } else {
                    while (i < content.size() && !std::isspace(static_cast<unsigned char>(content[i]))) {
                        value.push_back(content[i++]);
                    }
                }
            }

            if (!key.empty()) {
                node->attributes[key] = value;
            }
        }
    }

public:
    std::shared_ptr<Node> parse(const std::string& html) {
        auto root = std::make_shared<Node>();
        root->tag = "DOCUMENT_ROOT";

        std::stack<std::shared_ptr<Node>> nodeStack;
        nodeStack.push(root);

        size_t i = 0;
        while (i < html.size()) {
            if (html[i] == '<') {
                if (i + 3 < html.size() && html.compare(i, 4, "<!--") == 0) {
                    const size_t endComment = html.find("-->", i + 4);
                    i = (endComment == std::string::npos) ? html.size() : endComment + 3;
                    continue;
                }

                const size_t end = html.find('>', i);
                if (end == std::string::npos) {
                    break;
                }

                if (i + 1 < html.size() && html[i + 1] == '/') {
                    if (nodeStack.size() > 1) {
                        nodeStack.pop();
                    }
                    i = end + 1;
                    continue;
                }

                std::string content = html.substr(i + 1, end - i - 1);
                auto newNode = std::make_shared<Node>();
                parseTagContent(content, newNode);
                nodeStack.top()->children.push_back(newNode);

                const bool explicitSelfClose = !content.empty() && trim(content).back() == '/';
                if (!newNode->tag.empty() && !explicitSelfClose && !isSelfClosingTag(newNode->tag)) {
                    nodeStack.push(newNode);
                }
                i = end + 1;
            } else {
                size_t end = html.find('<', i);
                if (end == std::string::npos) {
                    end = html.size();
                }

                std::string text = html.substr(i, end - i);
                if (text.find_first_not_of(" \n\r\t") != std::string::npos) {
                    nodeStack.top()->raw_text += text;
                }
                i = end;
            }
        }

        return root;
    }
};

}  // namespace Kiseki

#endif
