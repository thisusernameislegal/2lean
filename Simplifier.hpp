#pragma once
#include "Group.hpp"
#include "NewLexer.hpp"
#include "NewSanitizer.hpp"

// Author Rainer Dylan Elyas
// Johan Iswara
// Vincent Liem
// Hans Sebastian

#include <unistd.h>
#include <iomanip>
#include <limits>

class Simplifier {

    Lexer lexer;
    Sanitizer sanitizer;
    std::string unique_variables;

    std::string minimize(std::vector<std::string> min) {
        if (min.empty()) return "0";

        #ifdef __DBG        
            std::cout << "Size: " << min.size() << ", Result: \n";
            for (const auto &m : min) std::cout << m << ' ';
            std::cout << '\n';
        #endif

        /** 
         * A + A = A 
         * A + a = 1
         * A + 0 = A
         * A & A = A
         * A & 1 = A
         * A & 0 = 0
         * A & a = 0
         */

        std::vector<std::string> canon_sop;
        std::stack<std::string> stck;
        for (const auto &i : min){
            stck.push(i);
            size_t hc = std::count_if(i.begin(), i.end(), [](char c) { return c == '#'; } );
            while (hc > 0){
                std::vector<std::string> perms;
                while (!stck.empty()) {
                    perms.push_back(stck.top());
                    stck.pop();
                }
                
            }
        }

        std::cout << "Canonized sop:\n";
        for (const auto &sop : canon_sop) std::cout << sop << ' ';
        std::cout << std::endl;
        return "";
    }



    std::vector<std::string> standardize(std::string sop){
        std::vector<std::string> tokens;
        std::array <char, 26> filter;
        size_t last_or = 0;
        filter.fill('#');
        for (size_t i = 0; i < sop.size(); i++){
            std::cout << "remaining: " << sop.substr(i, sop.size() - i) << "\n";
            if (islower(sop[i])) {
                size_t index = sop[i] - 'a';
                std::cout << "filter index: " << index << ", value: " << filter[index] << '\n';
                // std::cout << (islower(filter[index]) && isupper(sop[i])) << " " << (isupper(filter[index]) && islower(sop[i])) << '\n';
                if ((islower(filter[index])) && isupper(sop[i]) || (isupper(filter[index]) && islower(sop[i]))){
                    while (i < sop.size() && sop[i] != '+') i++;
                    std::cout << "Clearing filter and skipping the term\n";
                    for (const auto& i: filter) std::cout << i << ' ';
                    std::cout <<'\n';
                    filter.fill('#');
                    continue;
                }
                filter[index] = sop[i];
                for (const auto& i: filter) std::cout << i << ' ';
                std::cout <<'\n';
                continue;
            }
            if (isupper(sop[i])) {
                size_t index = sop[i] - 'A';
                std::cout << "filter index: " << index << ", value: " << filter[index] << '\n';
                // std::cout << (islower(filter[index]) && isupper(sop[i])) << " " << (isupper(filter[index]) && islower(sop[i])) << '\n';
                if ((islower(filter[index])) && isupper(sop[i]) || (isupper(filter[index]) && islower(sop[i]))) {
                    std::cout << "Clearing filter and skipping the term\n";
                    while (i < sop.size() && sop[i] != '+') i++;
                    for (const auto& i: filter) std::cout << i << ' ';
                    std::cout <<'\n';
                    filter.fill('#');
                    for (const auto& i: filter) std::cout << i << ' ';
                    std::cout <<'\n';
                    continue;
                }
                filter[index] = sop[i];
                for (const auto& i: filter) std::cout << i << ' ';
                std::cout <<'\n';
                continue;
            }
            if (sop[i] == '+'){
                last_or = i;
                std::string term;
                for (size_t k = 0; k < filter.size(); k++){
                    if (filter[k] != '#') {
                        size_t index = 0;
                        while (index < term.size() && toupper(term[index]) < toupper(filter[k])) index++;
                        term.insert(index, 1, filter[k]);
                    }
                }
                for (const auto& i: filter) std::cout << i << ' ';
                #ifdef __DBG
                    std::cout << "Term: " << term << '\n';
                #endif
                // if (!term.empty()) {
                    for (size_t k = 0; k < unique_variables.size(); k++){
                        if (k >= term.size()) {
                            term += '#';
                            continue;
                        }
                        if (k < term.size() && unique_variables[k] != toupper(term[k])) {
                            term.insert(k, 1, '#');
                        }
                    }
                    tokens.push_back(term);
                // }
                filter.fill('#');
            }
        }
        std::cout << "Final filter: " <<'\n';
        for (const auto& i: filter) std::cout << i << ' ';
        std::cout <<'\n';

        std::string term;
        for (size_t k = 0; k < filter.size(); k++){
            if (filter[k] != '#') {
                size_t index = 0;
                while (index < term.size() && toupper(term[index]) < toupper(filter[k])) index++;
                term.insert(index, 1, filter[k]);
            }
        }

        if(!term.empty()){
            std::cout << "Final: " << term << '\n';
            for (size_t k = 0; k < unique_variables.size(); k++){
                #ifdef __DBG
                        std::cout << "Term: " << term << '\n';
                    #endif
                if (k >= term.size()) {
                    term += '#';
                    continue;
                }
                if (k < term.size() && unique_variables[k] != toupper(term[k])) {
                    term.insert(k, 1, '#');
                }
            }
            std::cout << "pushing back :\"" << term << "\"\n";
            tokens.push_back(term);
        }

        #ifdef __DBG
            for (const auto &i :tokens) std::cout << i << ' ';
            std::cout << std::endl;
        #endif

        return tokens;
    }

    auto evaluate_postfix(TokenVector& v) {
        std::stack<std::string> tvstack;
        int counter = 0;
        for (auto &t : v){
            if (isalpha(t.value)){
                tvstack.push(std::string(1, t.value));
            } else {
                switch (t.value){
                    case '&':{
                        auto t1 = tvstack.top();
                        tvstack.pop();
                        auto t2 = tvstack.top();
                        tvstack.pop();
                        std::string res;
                        size_t s1 = 0, s2 = 0, e1 = 0, e2 = 0;
                        std::vector<std::string> indices1 = {}, indices2 = {};
                        for (; e1 < t1.size(); e1++){
                            if (t1[e1] == '+') {
                                #ifdef __DBG
                                    std::cout << "substring " << e1 << ": " << t1.substr(s1, e1 - s1) << '\n';
                                #endif
                                indices1.push_back(t1.substr(s1, e1 - s1));
                                s1 = e1 + 1;
                            }
                        }
                        #ifdef __DBG
                            std::cout << "Last substring: " << t1.substr(s1, e1 - s1) << '\n';
                        #endif
                        indices1.push_back(t1.substr(s1, e1 - s1));
                        for (; e2 < t2.size(); e2++){
                            if (t2[e2] == '+') {
                                indices2.push_back(t2.substr(s2, e2 - s2));
                                #ifdef __DBG
                                    std::cout << "substring " << e2 << ": " << t2.substr(s2, e2 - s2) << '\n';
                                #endif
                                s2 = e2 + 1;
                            }
                        }
                        #ifdef __DBG
                            std::cout << "Last substring: "  << t2.substr(s2, e2 - s2) << '\n';
                        #endif
                        indices2.push_back(t2.substr(s2, e2 - s2));
                        #ifdef __DBG
                            std::cout << "all substring: " << '\n';
                            for (const auto &i : indices1) std::cout << i << ' ';
                            std::cout << "\n";
                            for (const auto &i : indices2) std::cout << i << ' ';
                            std::cout << "\n";
                        #endif
                        if (indices1.size() > 1 && indices2.size() > 1) {
                            for (size_t i = 0; i < indices1.size() - 1; i++){
                                for (size_t j = 0; j < indices2.size() - 1; j++){
                                    res += indices1[i];
                                    res += '&';
                                    res += indices2[j];
                                    res += '+';
                                }
                                res += indices1[i];
                                res += '&';
                                res += indices2[indices2.size() - 1];
                                res += '+';
                            }
                            for (size_t j = 0; j < indices2.size() - 1; j++){
                                res += indices1[indices1.size() - 1];
                                res += '&';
                                res += indices2[j];
                                res += '+';
                            }
                            res += indices1[indices1.size() - 1];
                            res += '&';
                            res += indices2[indices2.size() - 1];
                        } else if (indices1.size() == 1 && indices2.size() > 1){
                            for (size_t j = 0; j < indices2.size() - 1; j++){
                                res += indices1[0];
                                res += '&';
                                res += indices2[j];
                                res += '+';
                            }
                            res += indices1[0];
                            res += '&';
                            res += indices2[indices2.size() - 1];
                        } else if (indices1.size() > 1 && indices2.size() == 1){
                            for (size_t j = 0; j < indices1.size() - 1; j++){
                                res += indices1[j];
                                res += '&';
                                res += indices2[0];
                                res += '+';
                            }
                            res += indices1[indices1.size() - 1];
                            res += '&';
                            res += indices2[0];
                        } else {
                            res += indices1[0];
                            res += '&';
                            res += indices2[0];
                        }
                        tvstack.push(res);
                        #ifdef __DBG
                        std::cout << "result: " << res << '\n';
                        #endif
                        counter++;
                    } break;
                    case '+': {
                        auto t1 = tvstack.top();
                        tvstack.pop();
                        #ifdef __DBG
                            std::cout << "Before: " << tvstack.top() << "\n";
                        #endif
                        tvstack.top() += '+';
                        tvstack.top() += t1; 
                        #ifdef __DBG
                            std::cout << "Result: " << tvstack.top() << "\n";
                        #endif
                    } break;
                    case '!': {
                        auto t1 = tvstack.top();
                        tvstack.pop();
                        std::stack<std::string> temp_stack;
                        size_t j = 0, i;
                        for (i = 0; i < t1.size(); i++){
                            if (std::isupper(t1[i])){
                                t1[i] = std::tolower(t1[i]);
                            } else if (std::islower(t1[i])){
                                t1[i] = std::toupper(t1[i]);
                            } else {
                                switch (t1[i]){
                                    case '&':{
                                        t1[i] = '+';
                                    } break;
                                    case '+': {
                                        temp_stack.push(t1.substr(j, i - j));
                                        #ifdef __DBG
                                        std::cout << "substring: " << t1.substr(j, i - j) << '\n';
                                        #endif
                                        j = i + 1;
                                    } break;
                                }
                            }
                        };
                        temp_stack.push(t1.substr(j, i - j));
                        while (temp_stack.size() > 1){
                            #ifdef __DBG
                                std::cout << temp_stack.top() << ' ';
                            #endif
                            auto t1 = temp_stack.top();
                            temp_stack.pop();
                            std::string t2 = temp_stack.top();
                            temp_stack.pop();
                            std::string res;
                            if (t1.empty() || (t2.empty())){
                                tvstack.push(std::string(""));
                                break;
                            }
                            if (t1[0] == '1') {
                                temp_stack.push(t2);
                                break;
                            }
                            if (t2[0] == '1'){
                                temp_stack.push(t1);
                                break;
                            }
                            size_t s1 = 0, s2 = 0, e1 = 0, e2 = 0;
                            std::vector<std::string> indices1 = {}, indices2 = {};
                            for (; e1 < t1.size(); e1++){
                                if (t1[e1] == '+') {
                                    #ifdef __DBG
                                        std::cout << "substring " << e1 << ": " << t1.substr(s1, e1 - s1) << '\n';
                                    #endif
                                    indices1.push_back(t1.substr(s1, e1 - s1));
                                    s1 = e1 + 1;
                                }
                            }
                            #ifdef __DBG
                                std::cout << "Last substring: " << t1.substr(s1, e1 - s1) << '\n';
                            #endif
                            indices1.push_back(t1.substr(s1, e1 - s1));
                            for (; e2 < t2.size(); e2++){
                                if (t2[e2] == '+') {
                                    indices2.push_back(t2.substr(s2, e2 - s2));
                                    #ifdef __DBG
                                        std::cout << "substring " << e2 << ": " << t2.substr(s2, e2 - s2) << '\n';
                                    #endif
                                    s2 = e2 + 1;
                                }
                            }
                            #ifdef __DBG
                                std::cout << "Last substring: "  << t2.substr(s2, e2 - s2) << '\n';
                            #endif
                            indices2.push_back(t2.substr(s2, e2 - s2));
                            #ifdef __DBG
                                std::cout << "all substring: " << '\n';
                                for (const auto &i : indices1) std::cout << i << ' ';
                                std::cout << "\n";
                                for (const auto &i : indices2) std::cout << i << ' ';
                                std::cout << "\n";
                            #endif
                            if (indices1.size() > 1 && indices2.size() > 1) {
                                for (size_t i = 0; i < indices1.size() - 1; i++){
                                    for (size_t j = 0; j < indices2.size() - 1; j++){
                                        res += indices1[i];
                                        res += '&';
                                        res += indices2[j];
                                        res += '+';
                                    }
                                    res += indices1[i];
                                    res += '&';
                                    res += indices2[indices2.size() - 1];
                                    res += '+';
                                }
                                for (size_t j = 0; j < indices2.size() - 1; j++){
                                    res += indices1[indices1.size() - 1];
                                    res += '&';
                                    res += indices2[j];
                                    res += '+';
                                }
                                res += indices1[indices1.size() - 1];
                                res += '&';
                                res += indices2[indices2.size() - 1];
                            } else if (indices1.size() == 1 && indices2.size() > 1){
                                for (size_t j = 0; j < indices2.size() - 1; j++){
                                    res += indices1[0];
                                    res += '&';
                                    res += indices2[j];
                                    res += '+';
                                }
                                res += indices1[0];
                                res += '&';
                                res += indices2[indices2.size() - 1];
                            } else if (indices1.size() > 1 && indices2.size() == 1){
                                for (size_t j = 0; j < indices1.size() - 1; j++){
                                    res += indices1[j];
                                    res += '&';
                                    res += indices2[0];
                                    res += '+';
                                }
                                res += indices1[indices1.size() - 1];
                                res += '&';
                                res += indices2[0];
                            } else {
                                res += indices1[0];
                                res += '&';
                                res += indices2[0];
                            }
                            temp_stack.push(res);
                        }
                        
                        std::cout << '\n';
                        tvstack.push(temp_stack.top());
                        #ifdef __DBG
                        std::cout << "negation result: " << temp_stack.top() << '\n';
                        #endif
                    } break;
                }
            }
        }
        return tvstack.top();   
    }

    TokenVector get_postfix(TokenQueue& q) {
        std::stack<Token> stack;
        TokenVector output_vec;
        while (!q.empty()) {
            Token f = q.front();
            switch (f.value) {
                case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
                case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
                case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': 
                case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd': 
                case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': 
                case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
                case 'q': case 'r': case 's': case 't': case 'u': case 'v':
                case 'w': case 'x': case 'y': case 'z': {
                    output_vec.push_back(f);
                } break;
                case '&': {
                    stack.push(f);
                } break;
                case '+': {
                    while (!stack.empty() && stack.top() != '(') {
                        output_vec.push_back(stack.top());
                        stack.pop();
                    }
                    stack.push(f);
                } break;
                case '!': 
                case '(': {
                    stack.push(f);
                } break;
                case ')': {
                    while (stack.top() != '('){
                        output_vec.push_back(stack.top());
                        stack.pop();
                    }
                    if (stack.top() == '(') {
                        stack.pop();
                    } 
                    if (!stack.empty() && stack.top() == '!') {
                        output_vec.push_back(stack.top());
                        stack.pop();
                    }
                } break;
                default:
                    break;
            }
            q.pop();
        }
        while (!stack.empty()) {
            output_vec.push_back(stack.top());
            stack.pop();   
        }
        #ifdef __DBG
            std::cout << "After postfix:\n";
            for (const auto& i : output_vec) {
                std::cout << '[' << i.value << "] ";
            }
            std::cout << std::endl;
        #endif
        return output_vec;
    }    
    public:
        Simplifier() : lexer(Lexer()), sanitizer(Sanitizer()) {}

        bool simplify(std::string str){
            std::string msg;
            auto tokens = lexer.parse_equation(str, msg);
            if (tokens.empty()) {
                std::cout << msg;
                msg.clear();
                return false;
                // exit(-1);
            }
            this->unique_variables = std::string(lexer.get_unique_variables());
            std::sort(this->unique_variables.begin(), this->unique_variables.end());
            #ifdef __DBG
                std::cout << "All unique variables: " << this->unique_variables << '\n';
            #endif
            auto filtered_queue = sanitizer.sanitize(tokens, msg);
            tokens = {};
            if (filtered_queue.empty()) {
                std::cout << msg;
                msg.clear();
                return false;
                // exit(-1);
            }
            // TokenVector expanded_expr = split(filtered_queue);
            TokenVector postfix_expr = get_postfix(filtered_queue);
            filtered_queue = {};     
            auto sop = evaluate_postfix(postfix_expr);
            auto standardized = standardize(sop);
            
            std::cout << "Result: ";
            for (const auto &i : standardized) {
                std::cout << i << ' '; 
            }
            std::cout << std::endl;
            
            // std::cout << minimize(standardized) << '\n';

            unique_variables.clear();
            return true;
        }
};