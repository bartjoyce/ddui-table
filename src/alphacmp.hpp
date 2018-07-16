//
//  alphacmp.hpp
//  Vidarr GUI
//
//  Created by Bartholomew Joyce on 16/07/2018.
//  Copyright © 2018 Bartholomew Joyce All rights reserved.
//

#ifndef ddui_table_alphacmp_hpp
#define ddui_table_alphacmp_hpp

#include <string>

namespace Table {

int alphacmp(const char *l, const char *r);
int alphacmp_std_string(const std::string& l, const std::string& r);
bool alphacmp_ascending(const std::string& l, const std::string& r);
bool alphacmp_descending(const std::string& l, const std::string& r);

struct alphacmp_operator {
    bool operator()(const std::string& l, const std::string& r) const {
        return alphacmp(l.c_str(), r.c_str()) < 0;
    }
};

}

#endif
