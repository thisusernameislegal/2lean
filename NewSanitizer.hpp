#pragma once
#include "Group.hpp"

using TokenVector = std::vector<Token>;
using TokenQueue = std::queue<Token>;

struct Sanitizer {
    TokenQueue sanitize(TokenVector& tokens, std::string& msg) {
        TokenQueue token_queue;
        Token last_token;
        for (const auto &token : tokens) {
            if (isalpha(token.value) && (isalpha(last_token.value) || last_token.value == ')')){
                // Error: variable or closing parenthesis followed by variable
                msg = "Unexpected variable [" ;
                msg += token.value + "].\n";
                return {};
            }
            else if (!isalpha(token.value)) {
                switch(token.value){
                    case '!':{
                    } break;
                    case '&':
                    case '+': {
                        if (last_token.value == '\0' || last_token.value == '(' || last_token == '&' || last_token == '+' || last_token == '!') {
                            // Error: operator followed by another operator / operator at the start of the equation.
                            msg = "Unexpected operator [";
                            msg += last_token.value ;
                            msg += "] followed by another operator ["; 
                            msg += token.value;
                            msg += "].\n";
                            return {};
                        }
                    } break;
                    case '(': {
                        if (isalpha(last_token.value)) {
                            // Error: variable or closing parenthesis followed by variable
                            msg = "Unexpected variable [";
                            msg += token.value + "] before '(' (try adding an operator between the variable and '(').\n";
                            return {};
                        } 
                    } break;
                    case ')': {
                        if (last_token.value == '(' || last_token.value == '&' || last_token.value == '+' || last_token.value == '!'){
                            // Error: Empty parenthesis / extraneous operator.
                            msg = "Found illegal syntax "; 
                            msg += last_token.value; 
                            msg += token.value + ".\n";
                            return {};
                        }
                    } break;
                }
            }
            last_token = token;
            token_queue.push(token);
            #ifdef __DBG
                Tokens.push_back(token);
            #endif
        }
        if (token_queue.back() == '+' || token_queue.back() == '&' || token_queue.back() == '!') {
            msg += "Found unpaired operator ";
            msg += token_queue.back().value;
            msg += '\n';
            return {};
        }

        #ifdef __DBG
            std::cout << "After sanitizing:\n";
            for (const auto &token : Tokens) {
                std::cout << ' ' << token.value << " ";
            }
            std::cout << std::endl;
        #endif
        
        Tokens.clear();

        return token_queue;
    }

    private:
        TokenVector Tokens;
};