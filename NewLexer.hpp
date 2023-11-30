#pragma once
#include "Group.hpp"

using TokenVector = std::vector<Token>;

struct Lexer {
    TokenVector parse_equation(std::string expr, std::string& msg) {
        if (expr.empty()) {
            msg = "No tokens to parse";
            return {};
        }
        TokenVector token_vec;
        uint8_t var_count = 0;
        int paren_count = 0;
        for (size_t index = 0; index < expr.size(); index++){
            #ifdef __DBG
                std::cout << "remaining: " << expr.substr(index, expr.size()) << '\n';
            #endif
            switch (expr[index]) {
                case '(': 
                    paren_count++;
                case '&': 
                case '+' : {
                    #ifdef __DBG
                        std::cout << "pushing token " << Token(expr[index]) << '\n';
                    #endif
                    token_vec.push_back(Token(expr[index]));
                } break;
                case '!' : {
                    size_t not_count = 1;
                    while (index + 1 < expr.size() && (expr[index + 1] == '!')) {
                        index++;
                        not_count++;
                    }
                    if (index + 1 >= expr.size() || (!isupper(expr[index + 1]) && expr[index + 1] != '(')) {
                        msg = "Expected token after ";
                        msg += expr[index];
                        msg += ", got nothing.\n";
                        return {};
                    }
                    #ifdef __DBG
                        std::cout << "not_count: " << not_count << '\n'; 
                    #endif
                    if (isupper(expr[index + 1])){
                        if (!filter[expr[index + 1] - 'A']) {
                            var_count++;
                            vars.push_back(expr[index + 1]);
                            filter[expr[index + 1] - 'A'] = true;
                            if (var_count > 10) {
                                msg = "Too many variables.\n";
                                return {};
                            }
                        }
                        if (not_count & 1) {
                            token_vec.push_back(Token(std::tolower(expr[index + 1])));
                        } else {
                            token_vec.push_back(Token(expr[index + 1]));
                        }
                        #ifdef __DBG
                            std::cout << "pushing token " << Token(expr[index + 1]) << '\n';
                        #endif
                        index++;
                        if (index + 1 < expr.size() && ( expr[index + 1] == '('  \
                            || expr[index + 1] == '!' || isupper(expr[index + 1]))) {
                            token_vec.push_back(Token('&'));
                            #ifdef __DBG
                                std::cout << "Found neighboring subexpression, pushing an AND operator.\n";
                            #endif
                        }
                    } else if (expr[index + 1] == '(') {
                        if (not_count & 1) {
                            token_vec.push_back(Token('!'));
                            #ifdef __DBG
                                std::cout << "pushing token " << Token('!') << '\n';
                            #endif
                        } 
                    } 
                } break;
                case ')': {
                    if (paren_count == 0) {
                        msg = "Found closing parenthesis without open pair.\n";
                        return {};
                    }
                    paren_count--;
                    token_vec.push_back(Token(expr[index]));

                    // Push and AND operator if there are variables / negation operator / open parenthesis next to each other.
                    if (index + 1 < expr.size() && (expr[index + 1] == '('  \
                        || expr[index + 1] == '!' || isupper(expr[index + 1]))) {
                        token_vec.push_back(Token('&'));
                        #ifdef __DBG
                            std::cout << "Found neighbouring subexpression, pushing an AND operator.\n";
                        #endif
                    }
                } break;
                case ' ': {
                    while (index < expr.size() && std::isspace(expr[index])) index++;
                    if (index < expr.size()) index--;
                } break;
                case 'A': case 'B': case 'C': case 'D': case 'E':  
                case 'F': case 'G': case 'H': case 'I': case 'J':
                case 'K': case 'L': case 'M': case 'N': case 'O': 
                case 'P': case 'Q': case 'R': case 'S': case 'T':
                case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z': {
                    // Count unique variables.
                    if (!filter[expr[index] - 'A']) {
                        // std::cout << "unique variable " << expr[index] << '\n';
                        var_count++;
                        vars.push_back(expr[index]);
                        filter[expr[index] - 'A'] = true;
                        if (var_count > 10) {
                            msg = "Too many variables.\n";
                            return {};
                        }
                    }

                    token_vec.push_back(Token(expr[index]));
                    #ifdef __DBG
                        std::cout << "pushing token " << Token(expr[index]) << '\n';
                    #endif

                    // Push and AND operator if there are variables / negation operator / open parenthesis next to each other.
                    if (index + 1 < expr.size() && (expr[index + 1] == '('  \
                        || expr[index + 1] == '!' || isupper(expr[index + 1]))) {
                        token_vec.push_back(Token('&'));
                        #ifdef __DBG
                            std::cout << "Found neighbouring subexpression, pushing an AND operator.\n";
                        #endif
                    }
                } break;
                default: {
                    msg = "Found unrecognized token (found [";
                    msg += expr[index];
                    msg += "]).\n";
                    return {};
                }
            }
        }
        
        #ifdef __DBG
        for (const auto &token : token_vec) {
            std::cout << '[' << token.value << "] ";
            // std::cout << token << ' ';
        }
        std::cout << std::endl;
        #endif
        if (paren_count > 0) {
            msg = "Found unterminated parenthesis.\n";
            return {};
        }
        return token_vec;
    }

    auto get_unique_variables() { 
        auto copy = this->vars;
        this->vars.clear();
        return copy;
    }
    auto get_filtered_variables() { 
        auto filter_copy = this->filter;
        this->filter.fill('\0');
        return filter_copy; 
    }
    
    private:
        std::array<char,26> filter;
        std::string errmsg;
        std::string vars;
};