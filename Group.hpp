#pragma once

#include <iostream>
#include <bitset>
#include <unordered_map>
#include <string>
#include <queue>
#include <stack>
#include <cstdint>
#include <array>
#include <algorithm>
#include <variant>
#include <type_traits>
#include <bits/ranges_algo.h>
#include <ranges>

#define UNREACHABLE std::cerr << "At file: " << __FILE__ << ":\n" << __LINE__ << " at " << __FUNCTION__ <<  " | UNREACHABLE " << std::endl, exit(1)
#define TODO(x) std::cerr << "At file: " << __FILE__ << ":\n" << __LINE__ << " at " << __FUNCTION__ << " | NOT IMPLEMENTED: " << x << std::endl, exit(-1)

struct Token {
    char value;

    Token() noexcept {}
    Token(char value) noexcept : value(value) {}

    friend bool operator==(const Token& e1, const Token& e2) noexcept {
        return e1.value == e2.value;
    }

    friend bool operator!=(const Token& e1, const Token& e2) noexcept {
        return e1.value != e2.value;
    }
    
    explicit operator char() const noexcept {
        return this->value;
    }

    friend bool operator<(const Token& e1, const Token& e2) noexcept {
        if (e1.value && !e2.value) return true;
        if (e2.value) return false;
        return e1.value < e2.value;
    }

    bool is_neg(const Token e2) const noexcept {
        return ((std::isupper(e2.value) && std::islower(this->value)) || (std::islower(e2.value) && std::isupper(this->value)));
    }

    void negate() noexcept {
        if (std::isalpha(this->value)){
            this->value ^= 0b100000;
            return;
        }
        switch (this->value) {
            case '+':
                this->value = '&';
                break;
            case '&':
                this->value = '+';
                break;
            default: {
                break;
            }
        }
    }

    Token get_negation() const noexcept {
        char negated_value;
        if (std::isalpha(this->value)){
            negated_value = (this->value ^ 0b100000);
            return Token(negated_value);
        }
        switch (this->value) {
            case '+':
                negated_value = '&';
                break;
            case '&':
                negated_value = '+';
                break;
            default: {
                break;
            }
        }
        return Token(negated_value);
    }
};

std::ostream& operator<<(std::ostream& os, const Token& expr){
    os << "[Type: ";
    if (std::islower(expr.value)) {
        os << "NVAR";
    } else if (std::isupper(expr.value)) {
        os << "VAR";
    } else {
        switch (expr.value) {
            case '+':
                os << "OR";
                break;
            case '&':
                os << "AND";
                break;
            case '(': 
                os << "OPAREN";
                break;
            case ')':
                os << "CPAREN";
                break;
            case '!':
                os << "NOT";
                break;
            default:
                os << "NONE";
                break;
        }
    }
    os << ", Value: " << expr.value << ']';
    return os;
}
    