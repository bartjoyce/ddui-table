//
//  settings.cpp
//  ddui-table
//
//  Created by Bartholomew Joyce on 10/07/2018.
//  Copyright © 2018 Bartholomew Joyce All rights reserved.
//

#include "settings.hpp"
#include <functional>
#include <algorithm>
#include <string.h>

namespace Table {

static Results apply_settings_grouped(Model& model, Settings& settings);

Results apply_settings(Model& model, Settings& settings) {

    if (settings.grouped_column != -1) {
        return apply_settings_grouped(model, settings);
    }

    Results results;
    
    auto num_cols = model.columns();
    auto num_rows = model.rows();
    
    // Step 1. Apply filters
    
    std::vector<bool> row_included(num_rows);
    std::fill(row_included.begin(), row_included.end(), true);
    
    for (int j = 0; j < num_cols; ++j) {
        if (!settings.filters[j].enabled) {
            continue;
        }
        
        auto& allowed_values = settings.filters[j].allowed_values;
        
        for (int i = 0; i < num_rows; ++i) {
            if (!row_included[i]) {
                continue;
            }
        
            auto cell = model.cell_text(i, j);
            row_included[i] = (allowed_values.find(cell) != allowed_values.end());
        }
    }
    
    for (int i = 0; i < num_rows; ++i) {
        if (row_included[i]) {
            results.row_indices.push_back(i);
        }
    }
    
    // Step 2. Apply sorting
    
    if (settings.sort_column != -1) {
        auto j = settings.sort_column;
    
        std::function<bool(int,int)> compare;
        if (settings.sort_ascending) {
            compare = [&](int i1, int i2) {
                return strcmp(model.cell_text(i1, j).c_str(), model.cell_text(i2, j).c_str()) < 0;
            };
        } else {
            compare = [&](int i1, int i2) {
                return strcmp(model.cell_text(i1, j).c_str(), model.cell_text(i2, j).c_str()) > 0;
            };
        }
        std::sort(results.row_indices.begin(), results.row_indices.end(), compare);
    }
    
    // Step 3. Apply column reordering, enabling
    
    for (int j = 0; j < num_cols; ++j) {
        auto col = settings.column_ordering[j];
        if (settings.column_enabled[col]) {
            results.column_indices.push_back(col);
        }
    }
    
    return results;
}

Results apply_settings_grouped(Model& model, Settings& settings) {
    Results results;
    
    auto num_cols = model.columns();
    auto num_rows = model.rows();
    
    // Step 1. Apply filters
    
    std::vector<bool> row_included(num_rows);
    std::fill(row_included.begin(), row_included.end(), true);
    
    for (int j = 0; j < num_cols; ++j) {
        if (!settings.filters[j].enabled) {
            continue;
        }
        
        auto& allowed_values = settings.filters[j].allowed_values;
        
        for (int i = 0; i < num_rows; ++i) {
            if (!row_included[i]) {
                continue;
            }
        
            auto cell = model.cell_text(i, j);
            row_included[i] = (allowed_values.find(cell) != allowed_values.end());
        }
    }

    // Step 2. Sort into groups

    auto j = settings.grouped_column;
    std::map<std::string, std::vector<int>> groups;
    for (int i = 0; i < num_rows; ++i) {
        if (!row_included[i]) {
            continue;
        }

        auto lookup = groups.find(model.cell_text(i, j));
        if (lookup == groups.end()) {
            groups.insert(std::make_pair(model.cell_text(i, j), std::vector<int> { i }));
        } else {
            lookup->second.push_back(i);
        }
    }
    
    // Step 3. Apply sorting to each group
    
    if (settings.sort_column != -1) {
        auto j = settings.sort_column;
    
        std::function<bool(int,int)> compare;
        if (settings.sort_ascending) {
            compare = [&](int i1, int i2) {
                return strcmp(model.cell_text(i1, j).c_str(), model.cell_text(i2, j).c_str()) < 0;
            };
        } else {
            compare = [&](int i1, int i2) {
                return strcmp(model.cell_text(i1, j).c_str(), model.cell_text(i2, j).c_str()) > 0;
            };
        }

        for (auto& pair : groups) {
            std::sort(pair.second.begin(), pair.second.end(), compare);
        }
    }

    // Step 4. Lay out all results linearly

    for (auto& pair : groups) {
        GroupHeading group_heading;
        group_heading.position = results.row_indices.size();
        group_heading.value = pair.first;
        group_heading.count = pair.second.size();
        results.group_headings.push_back(std::move(group_heading));

        results.row_indices.push_back(-1);

        if (settings.group_collapsed[pair.first]) {
            continue;
        }

        for (auto i : pair.second) {
            results.row_indices.push_back(i);
        }
    }
    
    // Step 5. Apply column reordering, enabling
    
    for (int j = 0; j < num_cols; ++j) {
        auto col = settings.column_ordering[j];
        if (col != settings.grouped_column && settings.column_enabled[col]) {
            results.column_indices.push_back(col);
        }
    }
    
    return results;
}

}
