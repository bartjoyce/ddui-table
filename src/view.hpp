//
//  view.hpp
//  ddui-table
//
//  Created by Bartholomew Joyce on 22/03/2018.
//  Copyright © 2018 Bartholomew Joyce All rights reserved.
//

#ifndef ddui_table_view_hpp
#define ddui_table_view_hpp

#include <ddui/Context>
#include <ddui/views/ScrollArea>
#include <ddui/views/ItemArranger>
#include <map>
#include "model.hpp"
#include "settings.hpp"

namespace Table {

struct TableState;

class TableItemArrangerModel : public ItemArranger::Model {
    public:
        int count();
        std::string label(int index);
        bool get_enabled(int index);
        void set_enabled(int index, bool enabled);
        void reorder(int old_index, int new_index);
  
        TableState* state;
};

struct TableState {
    TableState();

    Model* source;
    
    // Private copy of the data
    long private_copy_ref;
    std::vector<std::string> headers;
    std::vector<std::map<std::string, bool>> column_values;
    Settings settings;
    Results results;

    // Column resizing state
    struct {
        int active_column;
        int initial_width;
    } column_resizing;

    // UI info
    int content_width;
    int content_height;
    ScrollArea::ScrollAreaState scroll_area_state;
  
    // Column manager
    bool show_column_manager = false;
    TableItemArrangerModel item_arranger_model;
    ItemArranger::State item_arranger_state;

    // Filter overlay
    struct {
        int active_column;
        int x, y;
    } filter_overlay;
};

void update(TableState* table_state, Context ctx);

}

#endif
