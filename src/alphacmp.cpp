//
//  alphacmp.cpp
//  Vidarr GUI
//
//  Created by Bartholomew Joyce on 16/07/2018.
//  Copyright © 2018 Bartholomew Joyce All rights reserved.
//

#include "alphacmp.hpp"

namespace Table {

// Taken from: http://www.davekoelle.com/alphanum.html

int alphacmp_std_string(const std::string& l, const std::string& r) {
    return alphacmp(l.c_str(), r.c_str());
}

static inline bool is_digit(char ch) {
    return ch >= '0' && ch <= '9';
}

unsigned long read_number(const char** str) {
    const char* ch = *str;
    unsigned long value = 0;

    while (is_digit(*ch)) {
        value = value * 10 + (unsigned long)(*ch++ - '0');
    }

    *str = ch;
    return value;
}

int alphacmp(const char *l, const char *r) {
    enum mode_t { STRING, NUMBER } mode=STRING;

    while (*l && *r) {
        if (mode == STRING) {

            char l_char, r_char;
            while ((l_char=*l) && (r_char=*r)) {
                // check if this are digit characters
                const bool l_digit = is_digit(l_char), r_digit=is_digit(r_char);
                // if both characters are digits, we continue in NUMBER mode
                if (l_digit && r_digit) {
                    mode=NUMBER;
                    break;
                }
                // if only the left character is a digit, we have a result
                if(l_digit) return -1;
                // if only the right character is a digit, we have a result
                if(r_digit) return +1;
                // compute the difference of both characters
                const int diff = l_char - r_char;
                // if they differ we have a result
                if(diff != 0) return diff;
                // otherwise process the next characters
                ++l;
                ++r;
            }

        } else {
            // get the left number
            unsigned long l_int = read_number(&l);

            // get the right number
            unsigned long r_int = read_number(&r);

            // if the difference is not equal to zero, we have a comparison result
            const long diff=l_int-r_int;
            if (diff != 0) return diff;

            // otherwise we process the next substring in STRING mode
            mode=STRING;
        }
    }

    if(*r) return -1;
    if(*l) return +1;
    return 0;
}

bool alphacmp_ascending(const std::string& l, const std::string& r) {
    return alphacmp_std_string(l, r) < 0;
}
bool alphacmp_descending(const std::string& l, const std::string& r) {
    return alphacmp_std_string(l, r) > 0;
}

}
