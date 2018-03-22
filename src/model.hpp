//
//  model.hpp
//  ddui-table
//
//  Created by Bartholomew Joyce on 22/03/2018.
//  Copyright © 2018 Bartholomew Joyce All rights reserved.
//

#ifndef ddui_table_model_hpp
#define ddui_table_model_hpp

#include <vector>
#include <string>

namespace Table {

struct Model {
    Model();

    int version_count; // increments when state is changed
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> rows;
    std::vector<int> key;
};

int get_header_index(Model* model, std::string header);
void set_key(Model* model, std::vector<std::string> key);
void insert_row(Model* model, std::vector<std::string> row);

}

#endif
