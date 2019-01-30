//
//  export_table_to_csv.cpp
//  ddui-table
//
//  Created by Bartholomew Joyce on 30/01/2019.
//  Copyright © 2018 Bartholomew Joyce All rights reserved.
//

#include "export_table_to_csv.hpp"
#include <sstream>
#include <string.h>

namespace Table {

static void print_value_safe(std::stringstream& out, std::string value);

std::string export_table_to_csv(State* table) {

    auto& results = table->results;
    auto& model = *table->source;

    std::stringstream ss;

    // Step 1. Print table headings
    auto it = results.column_indices.begin();
    if (it < results.column_indices.end()) {
        print_value_safe(ss, model.header_text(*it));
        ++it;
    }
    for (; it < results.column_indices.end(); ++it) {
        ss << ',';
        print_value_safe(ss, model.header_text(*it));
    }
    ss << '\n';

    // Step 2. Print all the rows
    for (auto i : results.row_indices) {

        // Skip group headings
        if (i == -1) {
            continue;
        }

        auto it = results.column_indices.begin();
        if (it < results.column_indices.end()) {
            print_value_safe(ss, model.cell_text(i, *it));
            ++it;
        }
        for (; it < results.column_indices.end(); ++it) {
            ss << ',';
            print_value_safe(ss, model.cell_text(i, *it));
        }
        ss << '\n';
    }

    return ss.str();
}

static bool value_is_safe(std::string value) {
    for (auto i = 0; i < value.size(); ++i) {
        auto ch = value[i];
        if ((ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') ||
            (ch == ' ') || (ch == '.') ||
            (ch == '-') || (ch == '_') ||
            (ch == ':') || (ch == '+') ||
            (ch == '(') || (ch == ')')) {
            continue;
        } else {
            return false;
        }
    }
    return true;
}

void print_value_safe(std::stringstream& out, std::string value) {
    if (value_is_safe(value)) {
        out << value;
        return;
    }
    
    out << '"';
    for (auto i = 0; i < value.size(); ++i) {
        auto ch = value[i];
        if (ch == '\\') {
            out << "\\\\";
        } else if (ch == '"') {
            out << "\\\"";
        } else {
            out << ch;
        }
    }
    out << '"';
}

}
