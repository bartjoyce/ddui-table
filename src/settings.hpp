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

struct ColumnFilter {
    bool enabled;
    std::map<std::string, bool> allowed_values;
};

struct Settings {
    std::vector<float> column_widths;
    std::vector<bool> column_enabled;
    std::vector<int> column_ordering;

    int sort_column; // = -1 when unsorted
    bool sort_ascending;

    std::vector<ColumnFilter> filters;

    int grouped_column; // -1 when ungrouped
    std::map<std::string, bool> group_collapsed;
};

struct GroupHeading {
    int position;
    std::string value;
    int count;
};

struct Results {
    std::vector<int> column_indices;
    std::vector<int> row_indices;
    std::vector<GroupHeading> group_headings;
};

Results apply_settings(Model& model, Settings& settings);

}

#endif
