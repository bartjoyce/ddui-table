//
//  filter.hpp
//  ddui-table
//
//  Created by Bartholomew Joyce on 11/07/2018.
//  Copyright © 2018 Bartholomew Joyce All rights reserved.
//

#ifndef ddui_table_filter_hpp
#define ddui_table_filter_hpp

#include "view.hpp"

namespace Table {

void update_filter_overlay(State* state);
std::vector<std::string> prepare_filter_value_list(State* state, int column);

}

#endif
