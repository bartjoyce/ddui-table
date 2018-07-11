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

void refresh_results(State* state);
static void update_filter_buttons(State* state, Context ctx);
static void update_filter_values(State* state, Context ctx);
static void draw_filter_overlay_path(Context ctx, int x, int y);
static bool draw_filter_value(Context ctx, int y, bool active, const char* label);
static void get_box_position(Context ctx, int center_x, int center_y, int* box_x, int* box_y);

void update_filter_overlay(State* state, Context ctx) {
    using namespace style::filter_overlay;

    auto center_x = state->filter_overlay.x;
    auto center_y = state->filter_overlay.y;
    
    int box_x, box_y;
    get_box_position(ctx, center_x, center_y, &box_x, &box_y);

    // Fill in box background
    draw_filter_overlay_path(ctx, center_x, center_y);
    nvgFillColor(ctx.vg, COLOR_BG_BOX);
    nvgFill(ctx.vg);

    // Draw buttons
    {
        auto child_ctx = child_context(ctx, box_x, box_y, BOX_WIDTH, BUTTONS_AREA_HEIGHT);
        update_filter_buttons(state, child_ctx);
        nvgRestore(ctx.vg);
    }

    // Draw values
    {
        auto child_ctx = child_context(ctx, box_x, box_y + BUTTONS_AREA_HEIGHT, BOX_WIDTH,
                                       BOX_HEIGHT - BUTTONS_AREA_HEIGHT - BOX_BORDER_RADIUS);
        update_filter_values(state, child_ctx);
        nvgRestore(ctx.vg);
    }

    // Draw box outline
    draw_filter_overlay_path(ctx, center_x, center_y);
    nvgStrokeColor(ctx.vg, COLOR_OUTLINE);
    nvgStrokeWidth(ctx.vg, 1.0);
    nvgStroke(ctx.vg);
    
    // Catch clicks in the box
    if (mouse_hit(ctx, center_x - BOX_WIDTH / 2, center_y + ARROW_HEIGHT, BOX_WIDTH, BOX_HEIGHT)) {
        ctx.mouse->accepted = true;
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

bool draw_filter_button(Context ctx, int x, int y, int width, int height, ButtonForm form, bool active, const char* label) {
    using namespace style::filter_overlay;

    // Fill button background
    if (active) {
        nvgFillColor(ctx.vg, COLOR_BG_BUTTON_ACTIVE);
    } else {
        nvgFillColor(ctx.vg, COLOR_BG_BUTTON);
    }
    nvgBeginPath(ctx.vg);
    if (form == FORM_LEFT_MOST) {
        nvgRoundedRectVarying(ctx.vg, x, y, width, height, BUTTON_BORDER_RADIUS, 0, 0, BUTTON_BORDER_RADIUS);
    } else if (form == FORM_RIGHT_MOST) {
        nvgRoundedRectVarying(ctx.vg, x, y, width, height, 0, BUTTON_BORDER_RADIUS, BUTTON_BORDER_RADIUS, 0);
    } else {
        nvgRect(ctx.vg, x, y, width, height);
    }
    nvgFill(ctx.vg);

    // Draw button text
    nvgFillColor(ctx.vg, COLOR_TEXT_BUTTON);
    nvgFontFace(ctx.vg, "regular");
    nvgTextAlign(ctx.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgText(ctx.vg, x + width / 2, y + height / 2, label, NULL);
    nvgTextAlign(ctx.vg, NVG_ALIGN_LEFT);

    // Handle mouse hover
    if (mouse_over(ctx, x, y, width, height)) {
        *ctx.cursor = CURSOR_POINTING_HAND;
    }

    // Handle mouse click
    if (mouse_hit(ctx, x, y, width, height)) {
        ctx.mouse->accepted = true;
        return true;
    }
    return false;
}

void update_filter_buttons(State* state, Context ctx) {
    using namespace style::filter_overlay;
    
    auto& settings = state->settings;
    auto j = state->filter_overlay.active_column;
    
    auto button_height = ctx.height - 2 * BUTTONS_AREA_MARGIN;
    auto button_width_1 = (ctx.width - 2 * BUTTONS_AREA_MARGIN) / 3;
    auto button_width_3 = button_width_1;
    auto button_width_2 = ctx.width - 2 * BUTTONS_AREA_MARGIN - button_width_1 - button_width_3 - 2 * BUTTON_SPACING;
    
    auto y = BUTTONS_AREA_MARGIN;
    auto x1 = BUTTONS_AREA_MARGIN;
    auto x2 = x1 + button_width_1 + BUTTON_SPACING;
    auto x3 = x2 + button_width_2 + BUTTON_SPACING;
    
    auto ascending  = (settings.sort_column == j && settings.sort_ascending);
    auto descending = (settings.sort_column == j && !settings.sort_ascending);
    auto grouped = (settings.grouped_column == j);
    
    if (draw_filter_button(ctx, x1, y, button_width_1, button_height,
                           FORM_LEFT_MOST, ascending, "ASC")) {
        settings.sort_column = ascending ? -1 : j;
        settings.sort_ascending = true;
        if (grouped) {
            settings.grouped_column = -1;
            settings.group_collapsed.clear();
            Overlay::close((void*)state);
        }
        refresh_results(state);
    }

    if (draw_filter_button(ctx, x2, y, button_width_2, button_height,
                           FORM_MIDDLE, descending, "DESC")) {
        settings.sort_column = descending ? -1 : j;
        settings.sort_ascending = false;
        if (grouped) {
            settings.grouped_column = -1;
            settings.group_collapsed.clear();
            Overlay::close((void*)state);
        }
        refresh_results(state);
    }

    if (draw_filter_button(ctx, x3, y, button_width_3, button_height,
                           FORM_RIGHT_MOST, grouped, "GROUP")) {
        settings.grouped_column = grouped ? -1 : j;
        settings.group_collapsed.clear();
        if (ascending || descending) {
            settings.sort_column = -1;
        }
        refresh_results(state);
        Overlay::close((void*)state);
    }
    
}

void update_filter_values(State* state, Context ctx) {
    using namespace style::filter_overlay;
    
    auto inner_height = VALUE_HEIGHT * (1 + state->filter_overlay.value_list.size());

    ScrollArea::update(&state->filter_overlay.scroll_area_state, ctx, ctx.width, inner_height, [&](Context ctx) {
    
        auto j = state->filter_overlay.active_column;
        auto& value_list = state->filter_overlay.value_list;
        auto& filter = state->settings.filters[j];
        
        int y = 0;
        
        // "Select all" option
        {
            
            auto clicked = draw_filter_value(ctx, y, !filter.enabled, "Select all");
            if (clicked) {
                filter.enabled = !filter.enabled;
                filter.allowed_values.clear();
                refresh_results(state);
                *ctx.must_repaint = true;
            }
            
            y += VALUE_HEIGHT;
        }
        
        // Draw divider
        {
            nvgBeginPath(ctx.vg);
            nvgMoveTo(ctx.vg, VALUE_MARGIN, y);
            nvgLineTo(ctx.vg, ctx.width - VALUE_MARGIN, y);
            nvgStrokeColor(ctx.vg, COLOR_VALUE_SEPARATOR);
            nvgStrokeWidth(ctx.vg, 1.0);
            nvgStroke(ctx.vg);
        }
        
        // All value options
        bool value_was_clicked = false;
        std::string clicked_value;
        
        for (auto& value : value_list) {
            auto active = !filter.enabled || filter.allowed_values.find(value) != filter.allowed_values.end();
            
            auto clicked = draw_filter_value(ctx, y, active, value.c_str());
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
            *ctx.must_repaint = true;
        }
        
    });
}

void draw_filter_overlay_path(Context ctx, int x, int y) {
    using namespace style::filter_overlay;

    int bx, by;
    get_box_position(ctx, x, y, &bx, &by);

    auto w = BOX_WIDTH;
    auto h = BOX_HEIGHT;
    auto r = BOX_BORDER_RADIUS;
    
    nvgBeginPath(ctx.vg);
    nvgMoveTo(ctx.vg, x - ARROW_WIDTH, by);
    nvgLineTo(ctx.vg, x, y);
    nvgLineTo(ctx.vg, x + ARROW_WIDTH, by);
    nvgLineTo(ctx.vg, bx + w - r, by);
    nvgArcTo (ctx.vg, bx + w, by, bx + w, by + r, r);
    nvgLineTo(ctx.vg, bx + w, by + h - r);
    nvgArcTo (ctx.vg, bx + w, by + h, bx + w - r, by + h, r);
    nvgLineTo(ctx.vg, bx + r, by + h);
    nvgArcTo (ctx.vg, bx, by + h, bx, by + h - r, r);
    nvgLineTo(ctx.vg, bx, by + r);
    nvgArcTo (ctx.vg, bx, by, bx + r, by, r);
    nvgClosePath(ctx.vg);
}

bool draw_filter_value(Context ctx, int y, bool active, const char* label) {
    using namespace style::filter_overlay;
    
    nvgBeginPath(ctx.vg);
    nvgRoundedRect(ctx.vg, VALUE_MARGIN + VALUE_SQUARE_MARGIN,
                   y + (VALUE_HEIGHT - VALUE_SQUARE_SIZE) / 2, VALUE_SQUARE_SIZE,
                   VALUE_SQUARE_SIZE, VALUE_SQUARE_BORDER_RADIUS);
    if (active) {
        nvgFillColor(ctx.vg, style::COLOR_TEXT_HEADER);
        nvgFill(ctx.vg);
    } else {
        nvgStrokeColor(ctx.vg, style::COLOR_TEXT_ROW);
        nvgStrokeWidth(ctx.vg, 1.0);
        nvgStroke(ctx.vg);
    }

    if (active) {
        nvgFillColor(ctx.vg, style::COLOR_TEXT_HEADER);
        nvgFontFace(ctx.vg, "bold");
    } else {
        nvgFillColor(ctx.vg, style::COLOR_TEXT_ROW);
        nvgFontFace(ctx.vg, "regular");
    }
    int x = VALUE_MARGIN + VALUE_SQUARE_SIZE + 2 * VALUE_SQUARE_MARGIN;
    draw_text_in_box(ctx.vg, x, y, ctx.width - x - VALUE_MARGIN, VALUE_HEIGHT, label);
    
    if (mouse_over(ctx, VALUE_MARGIN, y, ctx.width - 2 * VALUE_MARGIN, VALUE_HEIGHT)) {
        *ctx.cursor = CURSOR_POINTING_HAND;
    }
    
    if (mouse_hit(ctx, VALUE_MARGIN, y, ctx.width - 2 * VALUE_MARGIN, VALUE_HEIGHT)) {
        ctx.mouse->accepted = true;
        return true;
    }
    
    return false;
}

void get_box_position(Context ctx, int center_x, int center_y, int* box_x, int* box_y) {
    using namespace style::filter_overlay;
    
    *box_x = center_x - BOX_WIDTH / 2;
    *box_y = center_y + ARROW_HEIGHT;
    
    if (*box_x > ctx.width - BOX_WIDTH - 5) {
        *box_x = ctx.width - BOX_WIDTH - 5;
    }
    if (*box_x < 5) {
        *box_x = 5;
    }
}

}
