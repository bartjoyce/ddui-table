//
//  filter.cpp
//  ddui-table
//
//  Created by Bartholomew Joyce on 11/07/2018.
//  Copyright © 2018 Bartholomew Joyce All rights reserved.
//

#include "filter.hpp"
#include "style.hpp"
#include <ddui/util/draw_text_in_box>
#include <ddui/views/Overlay>

namespace Table {

using namespace ddui;

void refresh_results(State* state);
static void update_filter_buttons(State* state);
static void update_filter_values(State* state);
static void draw_filter_overlay_path(float x, float y);
static bool draw_filter_value(float y, bool active, const char* label);
static void get_box_position(float center_x, float center_y, float* box_x, float* box_y);

void update_filter_overlay(State* state) {
    using namespace style::filter_overlay;

    auto center_x = state->filter_overlay.x;
    auto center_y = state->filter_overlay.y;
    
    float box_x, box_y;
    get_box_position(center_x, center_y, &box_x, &box_y);

    // Fill in box background
    draw_filter_overlay_path(center_x, center_y);
    fill_color(COLOR_BG_BOX);
    fill();

    // Draw buttons
    {
        sub_view(box_x, box_y, BOX_WIDTH, BUTTONS_AREA_HEIGHT);
        update_filter_buttons(state);
        restore();
    }

    // Draw values
    {
        sub_view(box_x, box_y + BUTTONS_AREA_HEIGHT, BOX_WIDTH,
                 BOX_HEIGHT - BUTTONS_AREA_HEIGHT - BOX_BORDER_RADIUS);
        update_filter_values(state);
        restore();
    }

    // Draw box outline
    draw_filter_overlay_path(center_x, center_y);
    stroke_color(COLOR_OUTLINE);
    stroke_width(1.0);
    stroke();
    
    // Catch clicks in the box
    if (mouse_hit(center_x - BOX_WIDTH / 2, center_y + ARROW_HEIGHT, BOX_WIDTH, BOX_HEIGHT)) {
        mouse_hit_accept();
    }
}

std::vector<std::string> prepare_filter_value_list(State* state, int column) {
    auto& values_existing = state->column_values[column];

    // For a disabled filter just show all existing values
    auto& filter = state->settings.filters[column];
    if (!filter.enabled) {
        std::vector<std::string> output;
        for (auto& pair : values_existing) {
            output.push_back(pair.first);
        }
        return output;
    }
    
    // For an enabled filter, find the intersection of
    // values_in_filter and values_existing.
    auto& values_in_filter = filter.allowed_values;
    for (auto& pair : values_in_filter) {
        pair.second = false;
    }
    
    for (auto& pair : values_existing) {
        auto lookup = values_in_filter.find(pair.first);
        if (lookup != values_in_filter.end()) {
            lookup->second = true;
        }
    }
    
    // Output non-existing values in the filter first
    std::vector<std::string> output;
    for (auto& pair : values_in_filter) {
        if (pair.second == false) {
            output.push_back(pair.first);
        }
    }
    for (auto& pair : values_existing) {
        output.push_back(pair.first);
    }
    
    return output;
}

enum ButtonForm {
    FORM_LEFT_MOST,
    FORM_MIDDLE,
    FORM_RIGHT_MOST
};

static bool draw_filter_button(float x, float y, float width, float height, ButtonForm form, bool active, const char* label) {
    using namespace style::filter_overlay;

    // Fill button background
    if (active) {
        fill_color(COLOR_BG_BUTTON_ACTIVE);
    } else {
        fill_color(COLOR_BG_BUTTON);
    }
    begin_path();
    if (form == FORM_LEFT_MOST) {
        rounded_rect_varying(x, y, width, height, BUTTON_BORDER_RADIUS, 0, 0, BUTTON_BORDER_RADIUS);
    } else if (form == FORM_RIGHT_MOST) {
        rounded_rect_varying(x, y, width, height, 0, BUTTON_BORDER_RADIUS, BUTTON_BORDER_RADIUS, 0);
    } else {
        rect(x, y, width, height);
    }
    fill();

    // Draw button text
    fill_color(COLOR_TEXT_BUTTON);
    font_face("regular");
    font_size(18.0);
    text_align(NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    text(x + width / 2, y + height / 2, label, NULL);
    text_align(NVG_ALIGN_LEFT);

    // Handle mouse hover
    if (mouse_over(x, y, width, height)) {
        set_cursor(CURSOR_POINTING_HAND);
    }

    // Handle mouse click
    if (mouse_hit(x, y, width, height)) {
        mouse_hit_accept();
        return true;
    }
    return false;
}

void update_filter_buttons(State* state) {
    using namespace style::filter_overlay;
    
    auto& settings = state->settings;
    auto j = state->filter_overlay.active_column;
    
    auto button_height = view.height - 2 * BUTTONS_AREA_MARGIN;
    auto button_width_1 = (view.width - 2 * BUTTONS_AREA_MARGIN) / 3;
    auto button_width_3 = button_width_1;
    auto button_width_2 = view.width - 2 * BUTTONS_AREA_MARGIN - button_width_1 - button_width_3 - 2 * BUTTON_SPACING;
    
    auto y = BUTTONS_AREA_MARGIN;
    auto x1 = BUTTONS_AREA_MARGIN;
    auto x2 = x1 + button_width_1 + BUTTON_SPACING;
    auto x3 = x2 + button_width_2 + BUTTON_SPACING;
    
    auto ascending  = (settings.sort_column == j && settings.sort_ascending);
    auto descending = (settings.sort_column == j && !settings.sort_ascending);
    auto grouped = (settings.grouped_column == j);
    
    if (draw_filter_button(x1, y, button_width_1, button_height,
                           FORM_LEFT_MOST, ascending, "ASC")) {
        state->settings_changed = true;
        settings.sort_column = ascending ? -1 : j;
        settings.sort_ascending = true;
        if (grouped) {
            settings.grouped_column = -1;
            settings.group_collapsed.clear();
            Overlay::close((void*)state);
        }
        refresh_results(state);
    }

    if (draw_filter_button(x2, y, button_width_2, button_height,
                           FORM_MIDDLE, descending, "DESC")) {
        state->settings_changed = true;
        settings.sort_column = descending ? -1 : j;
        settings.sort_ascending = false;
        if (grouped) {
            settings.grouped_column = -1;
            settings.group_collapsed.clear();
            Overlay::close((void*)state);
        }
        refresh_results(state);
    }

    if (draw_filter_button(x3, y, button_width_3, button_height,
                           FORM_RIGHT_MOST, grouped, "GROUP")) {
        state->settings_changed = true;
        settings.grouped_column = grouped ? -1 : j;
        settings.group_collapsed.clear();
        if (ascending || descending) {
            settings.sort_column = -1;
        }
        refresh_results(state);
        Overlay::close((void*)state);
    }
    
}

void update_filter_values(State* state) {
    using namespace style::filter_overlay;
    
    auto inner_height = VALUE_HEIGHT * (1 + state->filter_overlay.value_list.size());

    ScrollArea::update(&state->filter_overlay.scroll_area_state, view.width, inner_height, [&]() {
    
        auto j = state->filter_overlay.active_column;
        auto& value_list = state->filter_overlay.value_list;
        auto& filter = state->settings.filters[j];
        
        int y = 0;
        
        // "Select all" option
        {
            
            auto clicked = draw_filter_value(y, !filter.enabled, "Select all");
            if (clicked) {
                state->settings_changed = true;
                filter.enabled = !filter.enabled;
                filter.allowed_values.clear();
                refresh_results(state);
                repaint();
            }
            
            y += VALUE_HEIGHT;
        }
        
        // Draw divider
        {
            begin_path();
            move_to(VALUE_MARGIN, y);
            line_to(view.width - VALUE_MARGIN, y);
            stroke_color(COLOR_VALUE_SEPARATOR);
            stroke_width(1.0);
            stroke();
        }
        
        // All value options
        bool value_was_clicked = false;
        std::string clicked_value;
        
        for (auto& value : value_list) {
            auto active = !filter.enabled || filter.allowed_values.find(value) != filter.allowed_values.end();
            
            auto clicked = draw_filter_value(y, active, value.c_str());
            if (clicked) {
                value_was_clicked = true;
                clicked_value = value;
            }
            
            y += VALUE_HEIGHT;
        }
        
        if (value_was_clicked) {
            auto active = !filter.enabled || filter.allowed_values.find(clicked_value) != filter.allowed_values.end();
            auto value = clicked_value;
            
            if (!active) {
                // Add the value
                filter.allowed_values.insert(std::make_pair(value, true));
                    
                bool is_full = true;
                for (auto& value : value_list) {
                    if (filter.allowed_values.find(value) == filter.allowed_values.end()) {
                        is_full = false;
                        break;
                    }
                }
                
                if (is_full) {
                    // We've selected all values, disable the filter
                    filter.enabled = false;
                    filter.allowed_values.clear();
                }
            } else if (filter.enabled) {
                // Remove the value
                filter.allowed_values.erase(filter.allowed_values.find(value));
            } else {
                // Enable the filter, adding everything except 'value'
                filter.enabled = true;
                for (auto& value : value_list) {
                    if (value != clicked_value) {
                        filter.allowed_values.insert(std::make_pair(value, true));
                    }
                }
            }
            
            refresh_results(state);
            repaint();
        }
        
    });
}

void draw_filter_overlay_path(float x, float y) {
    using namespace style::filter_overlay;

    float bx, by;
    get_box_position(x, y, &bx, &by);

    auto w = BOX_WIDTH;
    auto h = BOX_HEIGHT;
    auto r = BOX_BORDER_RADIUS;
    
    begin_path();
    move_to(x - ARROW_WIDTH, by);
    line_to(x, y);
    line_to(x + ARROW_WIDTH, by);
    line_to(bx + w - r, by);
    arc_to (bx + w, by, bx + w, by + r, r);
    line_to(bx + w, by + h - r);
    arc_to (bx + w, by + h, bx + w - r, by + h, r);
    line_to(bx + r, by + h);
    arc_to (bx, by + h, bx, by + h - r, r);
    line_to(bx, by + r);
    arc_to (bx, by, bx + r, by, r);
    close_path();
}

bool draw_filter_value(float y, bool active, const char* label) {
    using namespace style::filter_overlay;
    
    begin_path();
    rounded_rect(VALUE_MARGIN + VALUE_SQUARE_MARGIN,
                 y + (VALUE_HEIGHT - VALUE_SQUARE_SIZE) / 2, VALUE_SQUARE_SIZE,
                 VALUE_SQUARE_SIZE, VALUE_SQUARE_BORDER_RADIUS);
    if (active) {
        fill_color(style::COLOR_TEXT_HEADER);
        fill();
    } else {
        stroke_color(style::COLOR_TEXT_ROW);
        stroke_width(1.0);
        stroke();
    }

    if (active) {
        fill_color(style::COLOR_TEXT_HEADER);
        font_face("bold");
    } else {
        fill_color(style::COLOR_TEXT_ROW);
        font_face("regular");
    }
    font_size(16.0);
    auto x = VALUE_MARGIN + VALUE_SQUARE_SIZE + 2 * VALUE_SQUARE_MARGIN;
    draw_text_in_box(x, y, view.width - x - VALUE_MARGIN, VALUE_HEIGHT, label);
    
    if (mouse_over(VALUE_MARGIN, y, view.width - 2 * VALUE_MARGIN, VALUE_HEIGHT)) {
        set_cursor(CURSOR_POINTING_HAND);
    }
    
    if (mouse_hit(VALUE_MARGIN, y, view.width - 2 * VALUE_MARGIN, VALUE_HEIGHT)) {
        mouse_hit_accept();
        return true;
    }
    
    return false;
}

void get_box_position(float center_x, float center_y, float* box_x, float* box_y) {
    using namespace style::filter_overlay;
    
    *box_x = center_x - BOX_WIDTH / 2;
    *box_y = center_y + ARROW_HEIGHT;
    
    if (*box_x > view.width - BOX_WIDTH - 5) {
        *box_x = view.width - BOX_WIDTH - 5;
    }
    if (*box_x < 5) {
        *box_x = 5;
    }
}

}
