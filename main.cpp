#include <iostream>
#include "Simplifier.hpp"

// Author Edison Widjaja
// Hans Sebastian

int main2(void){
    std::cout << "Hello user\nPress Ctrl + C on your keyboard to quit.\n";
    std::string in;
    while (1){
        std::cout << "Insert expression here: ";
        std::getline(std::cin, in, '\n');
        Simplifier simplifier;
        simplifier.simplify(in);
        in = "";
    }

    return 0;
}

int main(void){
    Simplifier simplifier;
    simplifier.simplify("A+ B  & C + D");
    return 0;
}