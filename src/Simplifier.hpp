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

    using ImplVector = std::vector<Implicant>;

    std::string minimize(std::vector<std::string> min) {
        if (min.empty()) return "0";

        #ifdef __DBG        
            std::cout << "minimizing...\n";
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

        struct ImplicantHash {
            size_t operator()(const Implicant& implicant) const {
                size_t h2 = 0, h1 = std::hash<std::string>{}(implicant.impl);    
                for (const auto &minterm : implicant.minterms) {
                    h2 ^= std::hash<size_t>{}(minterm.first);
                }
                return h1 ^ h2;
            }
        };

        std::unordered_map<std::string, std::string> canon_sop = {};
        {
            std::queue<std::string> queue;
            for (const auto &i : min){
                queue.push(i);
                size_t hc = std::count_if(i.begin(), i.end(), [](char c) { return c == '#'; } );
                if (hc != 0) {
                    std::queue<std::string> expanded = expand(queue, hc);
                    while (!expanded.empty()){
                        if (canon_sop[expanded.front()].empty()) canon_sop[expanded.front()] = expanded.front();
                        expanded.pop();
                    }
                } else {
                    if (canon_sop[i].empty()) canon_sop[i] = i;
                }
                queue.pop();
            }
        }
        
        #ifdef __DBG
            std::cout << "Canonized sop:\n";
            for (const auto &sop : canon_sop) std::cout << sop.second << ' ';
            std::cout << std::endl;
        #endif

        std::map<size_t, std::vector<Implicant>> min_group;

        for (auto &m : canon_sop){
            size_t min = 0, count = 0;
            for (size_t idx = 0; idx < unique_variables.size(); idx++) {
                min |= (m.second[idx] == unique_variables[idx]) << idx;
                count += (m.second[idx] == unique_variables[idx]);
            }
            #ifdef __DBG
                std::cout << "m: " << m.second << ", min: " << min << ", count: " << count << '\n';
            #endif
            min_group[count].push_back(Implicant(m.second, std::unordered_map<size_t, bool>{{min, true}}));
        }

        #ifdef __DBG
        for (const auto &k : min_group){
            std::cout << k.first << ": [\n";
            for (const auto &term : k.second) {
                if (term.impl.empty()) continue;
                size_t count = 0;
                std::cout << "\t[Value: " << term.impl << ", Minterms: {";
                auto i = term.minterms.begin();
                for (; i != term.minterms.end(); i++) {
                    count++;
                    if (count == term.minterms.size()) break;
                    std::cout << (*i).first << ", ";
                }
                std::cout << (*i).first << "}]\n"; 
            }
            std::cout <<"]\n";
        }
        #endif

        canon_sop = { };
        while (min_group.size() > 1){
            std::map<size_t, ImplVector> new_min_group;
            auto iter = min_group.begin(), second_iter = min_group.begin();
            second_iter++;
            bool all_primes = true;
            while (second_iter != min_group.end()) {
                auto &group1 = (*iter).second;
                auto &group2 = (*second_iter).second;
                std::unordered_map<std::string, bool> hash_set;
                for (auto &impls1 : group1){
                    bool combinable = impls1.combined;
                    for (auto &impls2 : group2) {
                        impls1.combined = false;
                        auto merge =  (impls1).combine(impls2);
                        #ifdef __DBG
                            std::cout << "Combining " << impls1.impl << " and " << impls2.impl << '\n';                        
                            std::cout << "Result: " << ((impls1).combined ? "Success" : "Fail") << '\n';
                        #endif
                        if ((impls1).combined) {
                            #ifdef __DBG
                                std::cout << "Inserting " << merge.impl << '\n';
                                std::cout << impls2.impl << ' ' << impls2.combined << '\n';
                            #endif
                            all_primes = false;
                            combinable = true;
                            if (!hash_set[merge.impl]) new_min_group[(*iter).first].push_back(merge);
                            hash_set[merge.impl] = true;
                        }
                    }
                    if (!combinable) new_min_group[(*iter).first].push_back(impls1);
                }
                #ifdef __DBG
                    std::cout << group2.back().impl << '\n';
                #endif
                iter++;
                second_iter++;
            }
            for (auto &last : (*iter).second) {
                std::cout << last.combined << '\n';
                if (last.combined) continue;
                new_min_group[(*iter).first].push_back(last); 
            }
            min_group = new_min_group;
            #ifdef __DBG
                std::cout << "After combining:\n";
                for (const auto &k : min_group){
                    std::cout << k.first << ": [\n";
                    for (const auto &term : k.second) {
                        if (term.impl.empty()) continue;
                        size_t count = 0;
                        std::cout << "\t[Value: " << term.impl << ", Minterms: {";
                        auto i = term.minterms.begin();
                        for (; i != term.minterms.end(); i++) {
                            count++;
                            if (count == term.minterms.size()) break;
                            std::cout << (*i).first << ", ";
                        }
                        std::cout << (*i).first << "}]\n"; 
                    }
                    std::cout <<"]\n";
                }
                std::cout << std::endl;
            #endif
            if (all_primes) break;
        }
        
        return get_essential(min_group); 
    }

    struct SetHash {
        size_t operator()(const std::unordered_set<std::string>& set) const {
            size_t hash = 0;
            for (const auto& element : set) {
                hash ^= std::hash<std::string>{}(element) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            }
            return hash;
        }
    };

    std::unordered_set<std::unordered_set<std::string>, SetHash> multiply(std::unordered_set<std::unordered_set<std::string>, SetHash>& expanded, std::unordered_set<std::string>& terms){
        std::unordered_set<std::unordered_set<std::string>, SetHash> expansion = {};
        std::unordered_set<std::unordered_set<std::string>, SetHash> split;
        // Split each term into its own set.
        for (const auto &t : terms){
            std::unordered_set<std::string> s;
            s.insert(t);
            split.insert(s);
        }
        // Merge the sets into one.
        for (const auto &exp_terms : expanded){
            for (const auto &s : split){
                std::unordered_set<std::string> temp = exp_terms;
                temp.insert(s.begin(), s.end());
                expansion.insert(temp);
            }
        }
        // Return the "multiplied" set.
        return expansion;
    }

    std::string get_essential(std::map<size_t, std::vector<Implicant>>& groups) {
        std::string res;
        std::unordered_map<size_t, std::unordered_set<std::string>> minterm_table;
        
        // Check if the result is a tautology.
        if (groups.size() == 1 && groups.begin()->second.size() == 1 && groups.begin()->second.begin()->minterms.size() == static_cast<size_t>(1 << (this->unique_variables.size()))) return "1";
        for (auto group = groups.begin(); group != groups.end(); group++) {
            auto implicants = group->second;
            for (auto implicant = implicants.begin(); implicant < implicants.end(); implicant++){
                for (auto minterm = implicant->minterms.begin(); minterm != implicant->minterms.end(); minterm++) {
                    minterm_table[minterm->first].insert(implicant->remove_hashtags());
                }
            }
        }
        
        #ifdef __DBG
            for (const auto &t : minterm_table) {
                std::cout << t.first << ": [";
                for (const auto  &l : t.second) std::cout << l << ' ';
                std::cout << "]\n";
            }
        #endif

        std::unordered_set<std::string> essentials;

        // Get all essential prime implicants.
        for (auto essential = minterm_table.begin(); essential != minterm_table.end(); essential++){
            if (essential->second.size() == 1) {
                #ifdef __DBG
                    std::cout << *(essential->second.begin()) << " is an essential prime implicant.\n";
                #endif
                essentials.insert(*essential->second.begin());
                continue;
            } 
        }

        std::unordered_map<size_t, std::unordered_set<std::string>> filtered_table;

        for (auto col = minterm_table.begin(); col != minterm_table.end(); col++) {
            bool found = false;
            for (const auto &es : essentials) {
                if (col->second.find(es) != col->second.end()) {
                    found = true;
                    #ifdef __DBG
                        std::cout << "Removing minterm {" << col->first << "} as it is covered by an essential minterm " << es << '\n';
                    #endif
                    break;
                }
            }
            if (found == true) continue;
            filtered_table[col->first] = col->second;
        }

        minterm_table = {};
        #ifdef __DBG
            std::cout << "Filtered amount: " << filtered_table.size() << '\n';
            for (const auto &t : filtered_table) {
                std::cout << t.first << ": [";
                for (const auto  &l : t.second) std::cout << l << ' ';
                std::cout << "]\n";
            }
        #endif

        {
            size_t essentials_count = 0;
            auto e = essentials.begin();
            for (; essentials_count < essentials.size() - 1; e++) {
                res += convert_to_std(*e);
                res += " + ";
                essentials_count++;
            }
            res += convert_to_std(*e);
        }
        
        if (!filtered_table.empty()) {
            std::string selectives;
            size_t selectives_count = 0;
            auto s = filtered_table.begin();
            
            std::unordered_set<std::unordered_set<std::string>, SetHash> set = { };
            
            for (const auto &t : s->second) {
                std::unordered_set<std::string> s;
                s.insert(t);
                set.insert(s);
            }
            #ifdef __DBG
                std::cout << "{\n";
                for (const auto &s : set) {
                    std::cout << "\t{";
                    for (const auto &str: s){
                        std::cout << "{\"" << str << "\"} ";
                    }
                    std::cout << "}\n";
                }
                std::cout << "}\n";
            #endif

            s++;
            while (s != filtered_table.end()){
                auto temp = multiply(set, s->second);
                set = temp;
                s++;
            }
            #ifdef __DBG
                std::cout << "{\n";
                for (const auto &s : set) {
                    std::cout << "\t{";
                    for (const auto &str: s){
                        std::cout << "{\"" << str << "\"} ";
                    }
                    std::cout << "}\n";
                }
                std::cout << "}\n";
            #endif
            std::multimap<size_t, std::unordered_set<std::string>> minims;

            for (const auto& s : set){ 
                minims.insert(std::make_pair(s.size(), s));
            }

            #ifdef __DBG
            for (const auto &m : minims) {
                for (const auto &s : m.second) {
                    std::cout << s << ' ';
                }
                std::cout << '\n';
            }
            #endif
            auto min = *(minims.begin());
            auto key = min.first;
            if (minims.count(key) == 1) {
                auto i = min.second.begin();
                for (; selectives_count != min.second.size() - 1; i++){
                    selectives += convert_to_std(*i);
                    selectives += " + ";
                } 
                selectives += convert_to_std(*i);
            } else {
                size_t candidates = 0, min_len = 0;
                auto iter = minims.begin();
                for (auto it = minims.begin(); candidates < minims.count(key); it++){
                    size_t len = 0;
                    for (const auto &i : it->second) len += i.length();
                    if (len < min_len) {
                        min_len = len;
                        iter = it;
                    }
                }
                auto i = iter->second.begin();
                for (; selectives_count != min.second.size() - 1; i++){
                    selectives += convert_to_std(*i);
                    selectives += " + ";
                } 
                selectives += convert_to_std(*i);          
            }
            res += " + ";
            res.append(selectives);
        }

        return res;
    }

    std::string convert_to_std(const std::string& s) const noexcept {
        std::string res;
        for (const auto &v : s){
            if (isupper(v)) res += v;
            if (islower(v)) {
                res += '!';
                res += toupper(v);
            } 
        }
        return res;
    }

    std::queue<std::string> expand(std::queue<std::string> term, size_t count){
        if (count == 0) return term;
        std::vector<std::string> cur;
        while (!term.empty()){
            cur.push_back(term.front());
            term.pop();
        }

        // Add a missing variable in place of the hashtags.
        for (auto &t : cur) {
            for (size_t idx = 0; idx < t.size(); idx++) {
                if (t[idx] == '#') {
                    t[idx] = this->unique_variables[idx];
                    #ifdef __DBG
                        std::cout << "pushing " << t << '\n';
                    #endif
                    term.push(t);
                    t[idx] = std::tolower(this->unique_variables[idx]);
                    term.push(t);
                    #ifdef __DBG
                        std::cout << "pushing " << t << '\n';
                    #endif
                    break;
                }
            }
        }
        
        return expand(term, count - 1);
    }

    std::vector<std::string> standardize(std::string sop){
        std::vector<std::string> tokens;
        std::array <char, 26> filter;
        filter.fill('#');
        for (size_t i = 0; i < sop.size(); i++){
            #ifdef __DBG
                std::cout << "remaining: " << sop.substr(i, sop.size() - i) << "\n";
            #endif
            if (sop[i] == '&') continue;
            if (islower(sop[i])) {
                size_t index = sop[i] - 'a';
                #ifdef __DBG
                    std::cout << "filter index: " << index << ", value: " << filter[index] << '\n';
                    std::cout << ((islower(filter[index]) && isupper(sop[i])) || (isupper(filter[index] && islower(sop[i]))) ? "Clearing..." : "Continuing...") << '\n';
                #endif
                if ((islower(filter[index]) && isupper(sop[i])) || (isupper(filter[index]) && islower(sop[i]))){
                    while (i < sop.size() && sop[i] != '+') i++;
                    #ifdef __DBG
                        std::cout << "Clearing filter and skipping the term\n";
                        for (const auto& i: filter) std::cout << i << ' ';
                        std::cout <<'\n';
                    #endif
                    filter.fill('#');
                    continue;
                }
                filter[index] = sop[i];
                #ifdef  __DBG
                    for (const auto& i: filter) std::cout << i << ' ';
                    std::cout <<'\n';
                #endif
                continue;
            }
            if (isupper(sop[i])) {
                size_t index = sop[i] - 'A';
                #ifdef __DBG    
                    std::cout << "filter index: " << index << ", value: " << filter[index] << '\n';
                    std::cout << ((islower(filter[index]) && isupper(sop[i])) || (isupper(filter[index]) && islower(sop[i])) ? "Clearing..." : "Continuing...") << '\n';
                #endif
                if ((islower(filter[index]) && isupper(sop[i])) || (isupper(filter[index]) && islower(sop[i]))) {
                    while (i < sop.size() && sop[i] != '+') i++;
                    #ifdef __DBG
                        std::cout << "Clearing filter and skipping the term\n";
                        for (const auto& i: filter) std::cout << i << ' ';
                        std::cout <<'\n';
                    #endif
                    filter.fill('#');
                    continue;
                }
                filter[index] = sop[i];
                #ifdef  __DBG
                    for (const auto& i: filter) std::cout << i << ' ';
                    std::cout <<'\n';
                #endif
                continue;
            }
            if (sop[i] == '+'){
                std::string term;
                for (size_t k = 0; k < filter.size(); k++){
                    if (filter[k] != '#') {
                        size_t index = 0;
                        while (index < term.size() && toupper(term[index]) < toupper(filter[k])) index++;
                        term.insert(index, 1, filter[k]);
                    }
                }
                #ifdef __DBG
                    for (const auto& i: filter) std::cout << i << ' ';
                    std::cout << "Term: " << term << '\n';
                #endif
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
                filter.fill('#');
            }
        }
        #ifdef __DBG
            std::cout << "Final filter: " <<'\n';
            for (const auto& i: filter) std::cout << i << ' ';
            std::cout <<'\n';
        #endif

        std::string term;
        for (size_t k = 0; k < filter.size(); k++){
            if (filter[k] != '#') {
                size_t index = 0;
                while (index < term.size() && toupper(term[index]) < toupper(filter[k])) index++;
                term.insert(index, 1, filter[k]);
            }
        }

        if(!term.empty()){
            #ifdef __DBG
                std::cout << "Final: " << term << '\n';
            #endif
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
            #ifdef __DBG
                std::cout << "pushing back :\"" << term << "\"\n";
            #endif
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

        std::string get_canon_sop(std::string str) {
            std::string msg;
            auto tokens = lexer.parse_equation(str, msg);
            if (tokens.empty()) {
                std::cout << msg;
                msg.clear();
                return "";
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
                return "";
                // exit(-1);
            }
            // TokenVector expanded_expr = split(filtered_queue);
            TokenVector postfix_expr = get_postfix(filtered_queue);
            filtered_queue = {};     
            auto sop = evaluate_postfix(postfix_expr);
            auto standardized = standardize(sop);
            std::vector<std::string> canon_sop;
            {
                std::queue<std::string> queue;
                for (const auto &i : standardized){
                    queue.push(i);
                    size_t hc = std::count_if(i.begin(), i.end(), [](char c) { return c == '#'; } );
                    if (hc != 0) {
                        std::queue<std::string> expanded = expand(queue, hc);
                        while (!expanded.empty()){
                            canon_sop.push_back(expanded.front());
                            expanded.pop();
                        }
                    }
                    queue.pop();
                }
            }
            std::string res;
            for (size_t i = 0; i < canon_sop.size() - 1; i++) {
                res += canon_sop[i];
                res += " + ";
            }
            res += canon_sop.back();
            return res;
        }

        bool simplify(std::string str){
            std::string msg;
            auto tokens = lexer.parse_equation(str, msg);
            if (tokens.empty()) {
                std::cout << msg;
                msg.clear();
                return false;
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
            }
            TokenVector postfix_expr = get_postfix(filtered_queue);
            filtered_queue = {};     
            auto sop = evaluate_postfix(postfix_expr);
            auto standardized = standardize(sop);
            auto minimized = minimize(standardized);

            std::cout << "Result: " << minimized << std::endl;

            unique_variables.clear();
            return true;
        }


};