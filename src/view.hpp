//
//  view.hpp
//  ddui-table
//
//  Created by Bartholomew Joyce on 22/03/2018.
//  Copyright © 2018 Bartholomew Joyce All rights reserved.
//

#ifndef ddui_table_view_hpp
#define ddui_table_view_hpp

#include <ddui/core>
#include <ddui/views/ScrollArea>
#include <ddui/views/ItemArranger>
#include <ddui/views/PlainTextBox>
#include <chrono>
#include <map>
#include "model.hpp"
#include "settings.hpp"

namespace Table {

struct State;

class TableItemArrangerModel : public ItemArranger::Model {
    public:
        int count();
        std::string label(int index);
        bool get_enabled(int index);
        void set_enabled(int index, bool enabled);
        void reorder(int old_index, int new_index);
  
        State* state;
};

struct State {
    State();

    Model* source;
    
    // Private copy of the data
    long private_copy_ref;
    std::vector<std::string> headers;
    std::vector<std::map<std::string, bool>> column_values;
    Settings settings;
    Results results;
    bool settings_changed;

    // Column resizing state
    struct {
        int active_column;
        float initial_width;
    } column_resizing;

    // UI info
    float content_width;
    float content_height;
    ScrollArea::ScrollAreaState scroll_area_state;
  
    // Column manager
    bool show_column_manager = false;
    TableItemArrangerModel item_arranger_model;
    ItemArranger::State item_arranger_state;

    // Filter overlay
    struct {
        int active_column;
        float x, y;
        std::vector<std::string> value_list;
        ScrollArea::ScrollAreaState scroll_area_state;
    } filter_overlay;

    // Selection state
    struct {
        int row, column;
        int candidate_row, candidate_column;
        std::vector<std::string> row_key;
        bool scroll_into_view;
    } selection;

    // Editable field
    struct {
        bool is_open;
        bool is_waiting_for_second_click;
        std::chrono::high_resolution_clock::time_point click_time;
        TextEdit::Model model;
        PlainTextBox::State state;
        int row, column;
        float cell_x, cell_y, cell_width;
        std::string current_cell_text;
    } editable_field;
};

void update(State* state);
bool process_settings_change(State* state);

}

#endif
