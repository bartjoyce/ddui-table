//
//  settings.cpp
//  ddui-table
//
//  Created by Bartholomew Joyce on 10/07/2018.
//  Copyright © 2018 Bartholomew Joyce All rights reserved.
//

#include "settings.hpp"
#include <algorithm>

namespace Table {

Results apply_settings(Model& model, Settings& settings) {
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
    
    
    // Step 3. Apply column reordering, enabling
    
    for (int j = 0; j < num_cols; ++j) {
        auto col = settings.column_ordering[j];
        if (settings.column_enabled[col]) {
            results.column_indices.push_back(col);
        }
    }
    
    return results;
}

}
