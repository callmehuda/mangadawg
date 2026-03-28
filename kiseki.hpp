#ifndef KISEKI_HPP
#define KISEKI_HPP

#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <memory>
#include <map>
#include <algorithm>

namespace Kiseki {

// Inherit ke enable_shared_from_this agar Node bisa mengembalikan dirinya sendiri (pointer)
struct Node : public std::enable_shared_from_this<Node> {
    std::string tag;
    std::string raw_text;
    std::map<std::string, std::string> attributes;
    std::vector<std::shared_ptr<Node>> children;

    // 1. Syntax ->text()
    std::string text() const {
        std::string result = raw_text;
        for (const auto& child : children) {
            result += child->text();
        }
        return result;
    }

    // 2. Syntax ->attr("nama_atribut")
    std::string attr(const std::string& key) const {
        auto it = attributes.find(key);
        if (it != attributes.end()) return it->second;
        return ""; // Return string kosong jika atribut tidak ada
    }

    // 3. Syntax ->find(".class") (Mencari di dalam node ini saja)
    std::vector<std::shared_ptr<Node>> find(const std::string& selector) {
        std::vector<std::shared_ptr<Node>> results;
        if (selector.empty()) return results;

        std::string type = "tag";
        std::string query = selector;

        if (selector[0] == '.') {
            type = "class";
            query = selector.substr(1);
        } else if (selector[0] == '#') {
            type = "id";
            query = selector.substr(1);
        }

        // Mulai pencarian dari anak-anak node ini
        for (auto& child : children) {
            child->searchNodes(type, query, results);
        }
        return results;
    }

private:
    // Fungsi internal untuk pencarian (Disembunyikan dari user)
    void searchNodes(const std::string& type, const std::string& query, std::vector<std::shared_ptr<Node>>& results) {
        bool match = false;
        if (type == "tag" && tag == query) {
            match = true;
        } else if (type == "id" && attr("id") == query) {
            match = true;
        } else if (type == "class" && attr("class").find(query) != std::string::npos) {
            match = true;
        }

        // Jika cocok, masukkan pointer diri sendiri ke hasil
        if (match) results.push_back(shared_from_this());

        // Lanjut cari ke cucu-cucunya
        for (auto& child : children) {
            child->searchNodes(type, query, results);
        }
    }
};

class Parser {
private:
    inline bool isSelfClosing(const std::string& tag) {
        // Daftar tag HTML yang tidak butuh tag penutup
        static const std::vector<std::string> sc_tags = {"img", "br", "hr", "input", "meta", "link", "source"};
        return std::find(sc_tags.begin(), sc_tags.end(), tag) != sc_tags.end();
    }

    inline void parseTagContent(const std::string& content, std::shared_ptr<Node> node) {
        size_t spacePos = content.find(' ');
        if (spacePos == std::string::npos) {
            node->tag = content;
            return;
        }
        
        node->tag = content.substr(0, spacePos);
        std::string attrStr = content.substr(spacePos + 1);
        size_t i = 0;
        
        while (i < attrStr.length()) {
            while (i < attrStr.length() && isspace(attrStr[i])) i++;
            if (i >= attrStr.length()) break;
            
            size_t eqPos = attrStr.find('=', i);
            if (eqPos == std::string::npos) break; 
            
            std::string key = attrStr.substr(i, eqPos - i);
            i = eqPos + 1;
            
            if (i < attrStr.length() && (attrStr[i] == '"' || attrStr[i] == '\'')) {
                char quote = attrStr[i];
                i++; 
                size_t endQuote = attrStr.find(quote, i);
                if (endQuote != std::string::npos) {
                    node->attributes[key] = attrStr.substr(i, endQuote - i);
                    i = endQuote + 1; 
                } else {
                    break;
                }
            }
        }
    }

public:
    inline std::shared_ptr<Node> parse(const std::string& html) {
        auto root = std::make_shared<Node>();
        root->tag = "DOCUMENT_ROOT";
        
        std::stack<std::shared_ptr<Node>> nodeStack;
        nodeStack.push(root);

        size_t i = 0;
        while (i < html.length()) {
            if (html[i] == '<') {
                if (i + 1 < html.length() && html[i+1] == '/') {
                    size_t end = html.find('>', i);
                    if (nodeStack.size() > 1) nodeStack.pop();
                    i = end + 1;
                } else {
                    size_t end = html.find('>', i);
                    std::string content = html.substr(i + 1, end - i - 1);
                    
                    auto newNode = std::make_shared<Node>();
                    parseTagContent(content, newNode);
                    
                    // Tambahkan node baru sebagai anak dari node di atas tumpukan
                    nodeStack.top()->children.push_back(newNode);
                    
                    // Cek apakah tag ini perlu dimasukkan ke tumpukan (butuh tag penutup)
                    // Jika tag diakhiri '/' (contoh: <img />) atau ada di daftar self-closing, jangan masukkan ke stack
                    if (!isSelfClosing(newNode->tag) && content.back() != '/') {
                        nodeStack.push(newNode);
                    }
                    
                    i = end + 1;
                }
            } else {
                size_t end = html.find('<', i);
                if (end == std::string::npos) end = html.length();
                
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

} // namespace kiseki

#endif // KISEKI_HPP
