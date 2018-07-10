//
//  settings.hpp
//  ddui-table
//
//  Created by Bartholomew Joyce on 10/07/2018.
//  Copyright © 2018 Bartholomew Joyce All rights reserved.
//

#ifndef ddui_table_settings_hpp
#define ddui_table_settings_hpp

#include "model.hpp"
#include <map>

namespace Table {

enum Ordering {
    UNORDERED,
    ORDER_ASCENDING,
    ORDER_DESCENDING
};

struct ColumnFilter {
    bool enabled;
    std::map<std::string, bool> allowed_values;
};

struct Settings {
    std::vector<int> column_widths;
    std::vector<bool> column_enabled;
    std::vector<int> column_ordering;

    Ordering sorting_order;
    int sorting_column;

    std::vector<ColumnFilter> filters;
};

struct Results {
    std::vector<int> column_indices;
    std::vector<int> row_indices;
};

Results apply_settings(Model& model, Settings& settings);

}

#endif
