//
//  view.cpp
//  ddui-table
//
//  Created by Bartholomew Joyce on 22/03/2018.
//  Copyright © 2018 Bartholomew Joyce All rights reserved.
//

#include "view.hpp"
#include "style.hpp"
#include "filter.hpp"
#include <ddui/util/draw_text_in_box>
#include <ddui/util/entypo>
#include <ddui/views/ContextMenu>
#include <ddui/views/Overlay>

namespace Table {

using namespace ddui;

static void refresh_model(State* state);
void refresh_results(State* state);
static float calculate_table_width(State* table_state);
static void update_function_bar(State* state, float* bar_height);
static void update_table_content(State* state, float outer_width, float outer_height);
static void update_column_separators(State* state);
static void update_group_headings(State* state);
static void update_table_headers(State* state);
static void update_column_header(State* state, int j, float& x, float y);
static void set_selection(State* state, int i, int j);
static void clear_selection(State* state);
static void stop_editing(State* state);
static void update_editable_field(State* state);

State::State() {

    source = NULL;
    private_copy_ref = -1;

    column_resizing.active_column = -1;
    filter_overlay.active_column = -1;

    content_width = 1;
    content_height = style::CELL_HEIGHT;

    selection.row = -1;
    selection.column = -1;

    item_arranger_model.state = this;

    item_arranger_state.model = &item_arranger_model;
    item_arranger_state.font_face = "regular";
    item_arranger_state.text_size = 14;
    item_arranger_state.color_background_enabled = rgb(76,  207, 255);
    item_arranger_state.color_text_enabled = rgb(0x000000);
    item_arranger_state.color_background_vacant = rgba(0xffffff, 0.3);
    
    editable_field.is_open = false;
    editable_field.is_waiting_for_second_click = false;
    editable_field.model.regular_font = "regular";
    TextEdit::set_style(&editable_field.model, false, 14, rgb(0x000000));

}

float calculate_table_width(State* state) {
    auto& settings = state->settings;
    auto& results = state->results;

    float width = style::SEPARATOR_WIDTH * results.column_indices.size();

    for (auto j : results.column_indices) {
        width += settings.column_widths[j];
    }

    return width;
}

void update(State* state) {
  
    register_focus_group(state);

    refresh_model(state);

    // Process key input
    if (has_key_event(state) &&
        state->selection.row != -1 &&
        (key_state.action == keyboard::ACTION_PRESS ||
         key_state.action == keyboard::ACTION_REPEAT)) {
        
        auto& selection = state->selection;
        bool super = key_state.mods & keyboard::MOD_SUPER;

        // Find the placement of the selection in the view
        auto max_row = state->results.row_indices.size() - 1;
        auto row_index = 0;
        for (; row_index <= max_row; ++row_index) {
            if (selection.row == state->results.row_indices[row_index]) {
                break;
            }
        }

        auto max_col = state->results.column_indices.size() - 1;
        auto col_index = 0;
        for (; col_index <= max_col; ++col_index) {
            if (selection.column == state->results.column_indices[col_index]) {
                break;
            }
        }
        
        // Update the index
        bool changed = true;
        switch (key_state.key) {
            case keyboard::KEY_UP: {
                auto new_row_index = row_index;
                if (super) new_row_index = 0;
                while (new_row_index > 0) {
                    --new_row_index;
                    if (state->results.row_indices[new_row_index] != -1) {
                        break;
                    }
                }
                if (state->results.row_indices[new_row_index] != -1) {
                    row_index = new_row_index;
                }
                break;
            }
            case keyboard::KEY_DOWN: {
                auto new_row_index = row_index;
                if (super) new_row_index = max_row;
                while (new_row_index < max_row) {
                    ++new_row_index;
                    if (state->results.row_indices[new_row_index] != -1) {
                        break;
                    }
                }
                if (state->results.row_indices[new_row_index] != -1) {
                    row_index = new_row_index;
                }
                break;
            }
            case keyboard::KEY_LEFT: {
                if (super) col_index = 0;
                if (col_index > 0) --col_index;
                break;
            }
            case keyboard::KEY_RIGHT: {
                if (super) col_index = max_col;
                if (col_index < max_col) ++col_index;
                break;
            }
            default:
                changed = false;
                break;
        }
        
        // Translate back to model selection
        if (changed) {
            consume_key_event();
            set_selection(state,
                          state->results.row_indices[row_index],
                          state->results.column_indices[col_index]);
        }

        // Copy from cell
        if (!changed && super && key_state.key == keyboard::KEY_C) {
            consume_key_event();
            auto i = state->results.row_indices[row_index];
            auto j = state->results.column_indices[col_index];
            auto cell = state->source->cell_text(i, j);
            set_clipboard_string(cell.c_str());
        }
        
        // Paste into cell
        if (!changed && super && key_state.key == keyboard::KEY_V) {
            consume_key_event();
            auto i = state->results.row_indices[row_index];
            auto j = state->results.column_indices[col_index];
            if (state->source->cell_editable(i, j)) {
                state->source->set_cell_text(i, j, get_clipboard_string());
            }
            repaint("Table::update(1)");
        }
    }
  
    // Process context menu
    ContextMenu::Handler cmh([&] (MenuBuilder::Menu& menu) {

        // Manage Columns item
        menu.item("Manage Columns")
            .checked(state->show_column_manager)
            .action([state]() {
                state->show_column_manager = !state->show_column_manager;
            });

        // Reset Grouping item
        if (state->settings.grouped_column != -1) {
            menu.item("Reset Grouping")
                .action([state]() {
                    state->settings_changed = true;
                    state->settings.grouped_column = -1;
                    state->settings.group_collapsed.clear();
                    refresh_results(state);
                });
        }

    });

    // Scroll the current selection into view
    if (state->selection.scroll_into_view && state->selection.row != -1) {
        state->selection.scroll_into_view = false;

        float x = 0, y = 0;

        auto sel_i = state->selection.row;
        auto sel_j = state->selection.column;
        
        const auto& settings = state->settings;
        const auto& results = state->results;

        for (auto j : results.column_indices) {
            if (j == sel_j) {
                break;
            }
            x += settings.column_widths[j] + style::SEPARATOR_WIDTH;
        }

        for (auto i : results.row_indices) {
            if (i == sel_i) {
                break;
            }
            y += style::CELL_HEIGHT;
        }

        ScrollArea::scroll_into_view(
            &state->scroll_area_state,
            x, y,
            settings.column_widths[sel_j], 2 * style::CELL_HEIGHT
        );
    }

    // Handle filter overlay
    Overlay::handle_overlay(state, std::bind(update_filter_overlay, state));
    if (!Overlay::is_open(state)) {
        state->filter_overlay.active_column = -1;
    }

    // Update function bar
    float function_bar_height;
    update_function_bar(state, &function_bar_height);

    // Update content
    sub_view(0, function_bar_height, view.width, view.height - function_bar_height);
    auto outer_width = view.width;
    auto outer_height = view.height;
    ScrollArea::update(&state->scroll_area_state,
                       state->content_width, state->content_height, [state, outer_width, outer_height]() {
        update_table_content(state, outer_width, outer_height);
        update_column_separators(state);
        update_group_headings(state);
        update_editable_field(state);
        update_table_headers(state);
    });
    restore();

    // Handle focusing on click
    if (did_blur(state) &&
        !has_focus(&state->editable_field.state) &&
        state->selection.row != -1) {
        clear_selection(state);
        refresh_results(state);
        repaint("Overlay::update(3)");
    }

    // Handle selection change
    if (state->selection.candidate_row != -1 && !mouse_state.accepted) {
        if (!has_focus(state)) {
            focus(state);
        }
        mouse_hit_accept();
        set_selection(state, state->selection.candidate_row,
                             state->selection.candidate_column);
        state->editable_field.is_waiting_for_second_click = true;
        state->editable_field.click_time = std::chrono::high_resolution_clock::now();
        refresh_results(state);
        repaint("Overlay::update(4)");
    }
    if (state->selection.row != -1 && mouse_hit(0, 0, view.width, view.height)) {
        if (!has_focus(state)) {
            focus(state);
        }
        mouse_hit_accept();
        clear_selection(state);
        refresh_results(state);
        repaint("Overlay::update(5)");
    }

}

void update_function_bar(State* state, float* bar_height) {

    float y = 0;
  
    if (state->show_column_manager) {
        constexpr float MARGIN = 8;
    
        begin_path();
        fill_color(style::COLOR_BG_HEADER);
        rect(0, y, view.width, state->item_arranger_state.content_height + 2 * MARGIN);
        fill();
      
        sub_view(MARGIN, y + MARGIN, view.width - 2 * MARGIN, view.height);
        ItemArranger::update(&state->item_arranger_state);
        restore();
      
        y += state->item_arranger_state.content_height + 2 * MARGIN;
    }

    *bar_height = y;

}

void update_table_content(State* state, float outer_width, float outer_height) {
    auto model = state->source;
    auto& settings = state->settings;
    auto& results = state->results;

    auto W = view.width;
    auto H = view.height;
    
    auto min_x = state->scroll_area_state.scroll_x;
    auto max_x = state->scroll_area_state.scroll_x + outer_width;
    auto min_y = state->scroll_area_state.scroll_y;
    auto max_y = state->scroll_area_state.scroll_y + outer_height;

    // Fill background
    begin_path();
    fill_color(style::COLOR_BG_ROW_EVEN);
    rect(0, 0, W, H);
    fill();

    // Set the text align to its default
    text_align(align::LEFT | align::BASELINE);

    // Odd row backgrounds
    fill_color(style::COLOR_BG_ROW_ODD);
    for (int y = style::CELL_HEIGHT * 2; y < H + style::CELL_HEIGHT; y += style::CELL_HEIGHT * 2) {
        begin_path();
        rect(0, y, W, style::CELL_HEIGHT);
        fill();
    }

    // Cells
    {
        font_size(style::TEXT_SIZE_ROW);
    
        int sel_i = state->selection.row;
        int sel_j = state->selection.column;

        state->selection.candidate_row = -1;
        state->selection.candidate_column = -1;

        int x = 0;
        for (int j : results.column_indices) {
            
            // Is this column visible? If not, skip it
            if (x > max_x || x + settings.column_widths[j] < min_x) {
                x += settings.column_widths[j] + style::SEPARATOR_WIDTH;
                continue;
            }
            
            int y = style::CELL_HEIGHT;
            for (int i : results.row_indices) {

                if (i == -1) {
                    // This row is a group heading
                    y += style::CELL_HEIGHT;
                    continue;
                }
                
                // Is this row visible? If not, skip it
                if (y > max_y || y + style::CELL_HEIGHT < min_y) {
                    y += style::CELL_HEIGHT;
                    continue;
                }
                
                // Is this cell selected? If it is, apply special styles
                auto is_selected = (sel_i == i && sel_j == j);
                if (is_selected) {
                    fill_color(style::COLOR_BG_CELL_ACTIVE);
                    begin_path();
                    rect(x, y, settings.column_widths[j], style::CELL_HEIGHT);
                    fill();
                    fill_color(style::COLOR_TEXT_ROW);
                    
                    state->editable_field.cell_x = x;
                    state->editable_field.cell_y = y;
                    state->editable_field.cell_width = settings.column_widths[j];
                }
                if (mouse_hit(x, y, settings.column_widths[j], style::CELL_HEIGHT)) {
                    state->selection.candidate_row = i;
                    state->selection.candidate_column = j;
                }

                fill_color(style::COLOR_TEXT_ROW);
                font_face("medium");
                sub_view(x, y, settings.column_widths[j], style::CELL_HEIGHT);
                {
                    auto result = model->render_cell(i, j, is_selected);
                    if (result == Model::USE_DEFAULT_RENDER) {
                        draw_centered_text_in_box(0, 0, view.width, view.height, model->cell_text(i, j).c_str());
                    }
                }
                restore();
                y += style::CELL_HEIGHT;
            }
            x += settings.column_widths[j] + style::SEPARATOR_WIDTH;
        }
    }
}

void update_table_headers(State* state) {
    auto& results = state->results;

    // Header background
    int header_y = state->scroll_area_state.scroll_y;
    begin_path();
    fill_color(style::COLOR_BG_HEADER);
    rect(0, header_y, view.width, style::CELL_HEIGHT);
    fill();

    // Header columns
    float x = 0;
    for (int j : results.column_indices) {
        update_column_header(state, j, x, header_y);
    }
}

void update_column_header(State* state, int j, float& x, float y) {
    auto& settings = state->settings;

    // Header option button
    int icon_size;
    {
        constexpr auto ICON_NONE = "⚡";
        constexpr auto ICON_ASC = "▾";
        constexpr auto ICON_DESC = "▴";
        constexpr auto ICON_FILT = "⏳";
        constexpr auto ICON_ASC_FILT = "▾⏳";
        constexpr auto ICON_DESC_FILT = "▴⏳";

        constexpr float MARGIN = 2.0;

        font_face("entypo");
        font_size(24.0);

        auto icon_text = (
            settings.filters[j].enabled ? (
                settings.sort_column == j ? (
                    settings.sort_ascending ? ICON_ASC_FILT : ICON_DESC_FILT
                ) : ICON_FILT
            ) : (
                settings.sort_column == j ? (
                    settings.sort_ascending ? ICON_ASC : ICON_DESC
                ) : ICON_NONE
            )
        );
        
        float ascender, descender, line_height;
        text_metrics(&ascender, &descender, &line_height);
        
        int text_y = y + (style::CELL_HEIGHT - (int)line_height) / 2 + (int)ascender;
        
        float bounds[4];
        text_bounds(0, 0, icon_text, 0, bounds);
        icon_size = bounds[2] - bounds[0] + 2 * MARGIN;
        
        if (settings.filters[j].enabled || settings.sort_column == j) {
            fill_color(style::COLOR_TEXT_HEADER);
        } else {
            fill_color(style::COLOR_BG_ROW_ODD);
        }
    
        if (mouse_over(x + settings.column_widths[j] - icon_size, y, icon_size, style::CELL_HEIGHT)) {
            set_cursor(CURSOR_POINTING_HAND);
        }

        if (mouse_hit(x + settings.column_widths[j] - icon_size, y, icon_size, style::CELL_HEIGHT)) {
            mouse_hit_accept();
            state->filter_overlay.active_column = j;
            to_global_position(&state->filter_overlay.x, &state->filter_overlay.y,
                               x + settings.column_widths[j] - icon_size / 2,
                               y + style::CELL_HEIGHT - 2 * MARGIN);
            state->filter_overlay.value_list = prepare_filter_value_list(state, j);
            state->filter_overlay.scroll_area_state = ScrollArea::ScrollAreaState();
            Overlay::open(state);
        }
        
        if (state->filter_overlay.active_column == j) {
            fill_color(style::COLOR_TEXT_ROW);
        }
        
        text(x + settings.column_widths[j] - icon_size + MARGIN, text_y, icon_text, NULL);
    }

    // Header labels
    {
        fill_color(style::COLOR_TEXT_HEADER);
        font_face("bold");
        font_size(style::TEXT_SIZE_HEADER);

        float ascender, descender, line_height;
        text_metrics(&ascender, &descender, &line_height);

        auto text_y = y + (style::CELL_HEIGHT - line_height) / 2 + ascender;
        
        constexpr float MARGIN = 2;
        
        auto content_std = state->source->header_text(j);
        auto content = content_std.c_str();
        auto width = settings.column_widths[j];
      
        auto length = content_std.length();
        char new_content[length + 4];
        auto text_width = truncate_text(width - 2 * MARGIN - icon_size, length, new_content, content);
      
        auto text_x = (width - text_width) / 2 + MARGIN;
        if (width - (text_x + text_width) < icon_size) {
            text_x = width - (icon_size + text_width);
        }

        text(x + text_x, text_y, new_content, 0);
    }
    
    x += settings.column_widths[j] + style::SEPARATOR_WIDTH;
}

void update_column_separators(State* state) {
    auto& settings = state->settings;
    auto& results = state->results;
    auto& column_resizing = state->column_resizing;

    auto H = view.height;

    // Column separators dragging
    int separator_y = style::CELL_HEIGHT + state->scroll_area_state.scroll_y;
    int separator_height = H - separator_y;
    if (column_resizing.active_column == -1) {
        int x = style::SEPARATOR_WIDTH / 2;
        column_resizing.active_column = -1;
        for (int j : results.column_indices) {
            x += settings.column_widths[j];
            if (mouse_over(x - 6, separator_y, 12, separator_height)) {
                set_cursor(CURSOR_HORIZONTAL_RESIZE);
            }
            if (mouse_hit(x - 6, separator_y, 12, separator_height)) {
                mouse_hit_accept();
                set_cursor(CURSOR_HORIZONTAL_RESIZE);
                column_resizing.active_column = j;
                column_resizing.initial_width = settings.column_widths[column_resizing.active_column];
                break;
            }
            x += style::SEPARATOR_WIDTH;
        }
    } else {
        if (mouse_state.pressed) {
            set_cursor(CURSOR_HORIZONTAL_RESIZE);
            float x, y, dx, dy;
            mouse_movement(&x, &y, &dx, &dy);
            float new_width = column_resizing.initial_width + dx;
            new_width = new_width > 50 ? new_width : 50;
            settings.column_widths[column_resizing.active_column] = new_width;
        } else {
            column_resizing.active_column = -1;
            state->settings_changed = true;
            state->content_width = calculate_table_width(state);
        }
    }

    // Column separators
    {
        float x = 0;
        for (auto j : results.column_indices) {
            x += settings.column_widths[j];
            begin_path();
            fill_color(j == column_resizing.active_column
                       ? style::COLOR_SEPARATOR_ACTIVE
                       : style::COLOR_SEPARATOR);
            rect(x, separator_y, style::SEPARATOR_WIDTH, separator_height);
            fill();
            x += style::SEPARATOR_WIDTH;
        }
    }
}

void update_group_headings(State* state) {
    if (state->settings.grouped_column == -1) {
        return;
    }

    constexpr float BUTTON_SIZE = 28;

    auto& model = *state->source;
    auto& settings = state->settings;
    auto& results = state->results;
    
    float ascender, descender, line_height;
    float bounds[4];
    
    // Measure expand/collapse button
    font_face("entypo");
    font_size(BUTTON_SIZE);
    
    text_metrics(&ascender, &descender, &line_height);
    text_bounds(0, 0, entypo::BLACK_DOWNPOINTING_SMALL_TRIANGLE, NULL, bounds);
    auto x = state->scroll_area_state.scroll_x;
    auto button_x = x + style::GROUP_HEADING_MARGIN;
    auto button_y = (style::CELL_HEIGHT - line_height) / 2 + ascender;
    auto button_width = bounds[2] - bounds[0];
    
    // Prepare & measure text
    auto header_text = model.header_text(settings.grouped_column);
    char buffer1[header_text.size() + 10];
    char buffer2[10];
    sprintf(buffer1, "  %s:  ", header_text.c_str());
    
    font_face("regular");
    font_size(style::TEXT_SIZE_GROUP_HEADING);
    text_metrics(&ascender, &descender, &line_height);
    text_bounds(0, 0, buffer1, NULL, bounds);
    auto text_y = (style::CELL_HEIGHT - line_height) / 2 + ascender;
    auto column_text_x = button_x + button_width;
    auto column_text_width = bounds[2] - bounds[0];
    
    for (auto& heading : results.group_headings) {
        auto collapsed = settings.group_collapsed[heading.value];
        auto y = (1 + heading.position) * style::CELL_HEIGHT;
        float clip_width, clip_height;
        get_clip_dimensions(&clip_width, &clip_height);
        
        // Fill background
        fill_color(style::COLOR_BG_GROUP_HEADING);
        begin_path();
        rect(x, y, clip_width, style::CELL_HEIGHT);
        fill();
        
        // Border line
        stroke_color(style::COLOR_SEPARATOR);
        stroke_width(1.0);
        begin_path();
        move_to(x, y);
        line_to(x + clip_width, y);
        stroke();
        
        // Draw expand/collapse button
        if (mouse_over(button_x - 2, y, button_width + 2, style::CELL_HEIGHT)) {
            set_cursor(CURSOR_POINTING_HAND);
            fill_color(style::COLOR_TEXT_GROUP_HEADING_HOVER);
        } else {
            fill_color(style::COLOR_TEXT_GROUP_HEADING);
        }
        if (mouse_hit(button_x - 2, y, button_width + 2, style::CELL_HEIGHT)) {
            mouse_hit_accept();
            state->settings_changed = true;
            settings.group_collapsed[heading.value] = !collapsed;
            refresh_results(state);
            repaint("Overlay::update_group_headings");
            return;
        }
        font_face("entypo");
        font_size(BUTTON_SIZE);
        text(button_x, y + button_y,
             collapsed ? entypo::BLACK_RIGHTPOINTING_SMALL_TRIANGLE
                       : entypo::BLACK_DOWNPOINTING_SMALL_TRIANGLE, NULL);
        
        // Draw column title
        if (mouse_over(column_text_x, y, column_text_width, style::CELL_HEIGHT)) {
            set_cursor(CURSOR_POINTING_HAND);
            fill_color(style::COLOR_TEXT_GROUP_HEADING_HOVER);
        } else {
            fill_color(style::COLOR_TEXT_GROUP_HEADING);
        }
        if (mouse_hit(column_text_x, y, column_text_width, style::CELL_HEIGHT)) {
            mouse_hit_accept();
            state->filter_overlay.active_column = settings.grouped_column;
            to_global_position(&state->filter_overlay.x, &state->filter_overlay.y,
                               column_text_x + column_text_width / 2,
                               y + style::CELL_HEIGHT - 2);
            state->filter_overlay.value_list = prepare_filter_value_list(state, settings.grouped_column);
            state->filter_overlay.scroll_area_state = ScrollArea::ScrollAreaState();
            Overlay::open(state);
        }
        font_face("regular");
        font_size(style::TEXT_SIZE_GROUP_HEADING);
        text(column_text_x, y + text_y, buffer1, NULL);
        
        // Draw value text
        font_face("bold");
        fill_color(style::COLOR_TEXT_GROUP_HEADING);
        text_bounds(0, 0, heading.value.c_str(), NULL, bounds);
        auto value_text_x = column_text_x + column_text_width;
        auto value_text_width = bounds[2] - bounds[0];
        text(value_text_x, y + text_y, heading.value.c_str(), NULL);
        
        // Draw count text
        font_face("regular");
        auto count_text_x = value_text_x + value_text_width;
        sprintf(buffer2, " (%d)", heading.count);
        text(count_text_x, y + text_y, buffer2, NULL);
    }
    
}

void refresh_model(State* state) {
    auto model = state->source;
    if (model == NULL) {
        return; // No source data to refresh
    }

    if (model->ref() == state->private_copy_ref) {
        return; // Model is up-to-date
    }
    
    auto& settings = state->settings;

    // Check if headers have changed
    bool headers_changed = false;
    if (state->headers.size() != model->columns()) {
        headers_changed = true;
    } else {
        for (int j = 0; j < model->columns(); ++j) {
            if (state->headers[j] != model->header_text(j)) {
                headers_changed = true;
                break;
            }
        }
    }

    // Reset settings if headers have changed
    if (headers_changed) {
        
        auto num_cols = model->columns();

        state->settings_changed = true;
    
        settings.column_widths.clear();
        while (settings.column_widths.size() < num_cols) {
            settings.column_widths.push_back(style::CELL_WIDTH_INITIAL);
        }

        settings.column_enabled.clear();
        while (settings.column_enabled.size() < num_cols) {
            settings.column_enabled.push_back(true);
        }
      
        settings.column_ordering.clear();
        while (settings.column_ordering.size() < num_cols) {
            settings.column_ordering.push_back(settings.column_ordering.size());
        }
        
        ColumnFilter empty_filter;
        empty_filter.enabled = false;
        
        settings.filters.clear();
        while (settings.filters.size() < num_cols) {
            settings.filters.push_back(empty_filter);
        }
        
        settings.sort_column = -1;

        settings.grouped_column = -1;
        
        state->headers.clear();
        for (int j = 0; j < num_cols; ++j) {
            state->headers.push_back(model->header_text(j));
        }

        clear_selection(state);
    }

    // Find all column values
    state->column_values.clear();
    for (int j = 0; j < model->columns(); ++j) {
        std::map<std::string, bool> values;
        
        for (int i = 0; i < model->rows(); ++i) {
            if (values.find(model->cell_text(i, j)) == values.end()) {
                values.insert(std::make_pair(model->cell_text(i, j), true));
            }
        }
        
        state->column_values.push_back(std::move(values));
    }
    
    // If the overlay is open, update the value list
    if (state->filter_overlay.active_column != -1) {
        state->filter_overlay.value_list = prepare_filter_value_list(state, state->filter_overlay.active_column);
    }

    // If there's an active selection, update it
    if (state->selection.row != -1) {
        auto key = model->key();

        // No key, simply index based
        if (key.empty()) {
            
            // Reset the selection if the row doesn't exist
            if (state->selection.row >= model->rows()) {
                clear_selection(state);
            }
        } else {

            // Since it's key-based, we have to find the new
            // index.
            bool found = false;
            for (int i = 0; i < model->rows(); ++i) {
                bool match = true;
                for (int j = 0; j < key.size(); ++j) {
                    if (model->cell_text(i, key[j]) != state->selection.row_key[j]) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    found = true;
                    if (state->selection.row != i) {
                        set_selection(state, i, state->selection.column);
                    } 
                    break;
                }
            }

            // Row no longer exists, clear the selection
            if (!found) {
                clear_selection(state);
            }
        }
    }

    state->private_copy_ref = model->ref();

    refresh_results(state);
}

void refresh_results(State* state) {
    auto model = state->source;
    auto& settings = state->settings;

    // If we're grouping, update the group_collapsed map
    if (settings.grouped_column != -1) {

        auto j = settings.grouped_column;

        auto previous_group_collapsed = std::move(settings.group_collapsed);
        auto next_group_collapsed = std::map<std::string, bool>();

        for (int i = 0; i < model->rows(); ++i) {
            auto value = model->cell_text(i, j);
            auto lookup = previous_group_collapsed.find(value);
            if (lookup == previous_group_collapsed.end()) {
                next_group_collapsed.insert(std::make_pair(value, false));
            } else {
                next_group_collapsed.insert(std::make_pair(value, lookup->second));
            }
        }

        settings.group_collapsed = std::move(next_group_collapsed);

    }

    // (Re)apply the settings
    state->results = apply_settings(*model, settings);

    // Compute dimensions for scroll area
    state->content_width = calculate_table_width(state);
    state->content_height = style::CELL_HEIGHT * (state->results.row_indices.size() + 1);

    // If there's a selection, confirm that it's included in the result
    if (state->selection.row != -1) {
        bool found_row = false;
        for (int i : state->results.row_indices) {
            if (state->selection.row == i) {
                found_row = true;
                break;
            }
        }

        bool found_col = false;
        for (int j : state->results.column_indices) {
            if (state->selection.column == j) {
                found_col = true;
                break;
            }
        }

        // If the selection is not visible in the result grid, clear it
        if (!found_row || !found_col) {
            clear_selection(state);
        }
    }
}

bool process_settings_change(State* state) {
    auto settings_changed = state->settings_changed;
    state->settings_changed = false;
    return settings_changed;
}

void set_selection(State* state, int i, int j) {
    state->selection.row = i;
    state->selection.column = j;
    state->selection.row_key.clear();
    state->selection.scroll_into_view = true;

    auto key = state->source->key();
    if (!key.empty()) {
        auto i = state->selection.row;
        for (auto j : key) {
            state->selection.row_key.push_back(state->source->cell_text(i, j));
        }
    }

    stop_editing(state);
}

void clear_selection(State* state) {
    state->selection.row = -1;
    state->selection.column = -1;
    state->selection.row_key.clear();

    stop_editing(state);
}

void stop_editing(State* state) {
    if (!state->editable_field.is_open) {
        return;
    }

    state->editable_field.is_open = false;
    
    auto i = state->editable_field.row;
    auto j = state->editable_field.column;
    
    TextEdit::Selection selection = { 0 };
    selection.b_index = state->editable_field.model.lines.front().characters.size();
    
    auto buffer = TextEdit::get_text_content(&state->editable_field.model, selection);
    auto value = std::string(buffer.get());
    
    if (state->editable_field.current_cell_text != value) {
        state->source->set_cell_text(i, j, value);
    }
}

void update_editable_field(State* state) {
    // Dealing with the double-click
    bool should_open = false;
    if (state->editable_field.is_waiting_for_second_click &&
        mouse_hit(state->editable_field.cell_x, state->editable_field.cell_y,
                  state->editable_field.cell_width, style::CELL_HEIGHT)) {

        mouse_hit_accept();

        auto current_time = std::chrono::high_resolution_clock::now();
        auto time_elapsed = std::chrono::duration_cast<std::chrono::duration<double>>
                            (current_time - state->editable_field.click_time).count();
        state->editable_field.is_waiting_for_second_click = false;

        if (time_elapsed < 0.5) {
            should_open = true;
        }
    }

    // Dealing with keyboard
    if (state->selection.row != -1 && has_key_event(state)) {
        if (key_state.action == keyboard::ACTION_PRESS &&
            key_state.key == keyboard::KEY_ENTER) {
            consume_key_event();
        }
        if (key_state.action == keyboard::ACTION_RELEASE &&
            key_state.key == keyboard::KEY_ENTER) {
            stop_editing(state);
            consume_key_event();
            should_open = true;
        }
    }

    // Open the editable field if either a double-click happened OR enter key pressed
    if (should_open) {
        auto i = state->selection.row;
        auto j = state->selection.column;
        if (!state->source->cell_editable(i, j)) {
            return;
        }

        state->editable_field.is_open = true;
        state->editable_field.row = i;
        state->editable_field.column = j;
        state->editable_field.current_cell_text = state->source->cell_text(i, j);
        TextEdit::set_text_content(&state->editable_field.model,
                                   state->editable_field.current_cell_text.c_str());
        focus(&state->editable_field.state);
    }

    // It's not open!
    if (!state->editable_field.is_open) {
        return;
    }
    
    // If the cell text changes whilst editing, close the field
    {
        auto i = state->editable_field.row;
        auto j = state->editable_field.column;
        if (state->editable_field.current_cell_text != state->source->cell_text(i, j)) {
            state->editable_field.is_open = false;
            return;
        }
    }

    if (has_key_event(&state->editable_field.state)) {
        if (key_state.action == keyboard::ACTION_PRESS &&
            key_state.key == keyboard::KEY_ENTER) {
            consume_key_event();
        }
        if (key_state.action == keyboard::ACTION_RELEASE &&
            key_state.key == keyboard::KEY_ENTER) {
            stop_editing(state);
            consume_key_event();
            focus(state);
            return;
        }
    }

    sub_view(state->editable_field.cell_x, state->editable_field.cell_y,
             state->editable_field.cell_width, style::CELL_HEIGHT);
    {
        auto style = *PlainTextBox::get_global_styles();
        style.border_radius = 0;
        style.margin = 6;

        PlainTextBox(&state->editable_field.state, &state->editable_field.model)
            .set_styles(&style)
            .update();
    }
    restore();
    
    // Select all on focus
    if (did_focus(&state->editable_field.state)) {
        auto& model = state->editable_field.model;
        model.selection = { 0 };
        model.selection.b_index = model.lines.front().characters.size();
        model.version_count++;
        repaint("Overlay::update_editable_field(1)");
    }
    
    // Close the box on blur
    if (did_blur(&state->editable_field.state)) {
        stop_editing(state);
        repaint("Overlay::update_editable_field(2)");
    }

}

int TableItemArrangerModel::count() {
    return state->source->columns();
}
std::string TableItemArrangerModel::label(int index) {
    return state->source->header_text(state->settings.column_ordering[index]);
}
bool TableItemArrangerModel::get_enabled(int index) {
    auto j = state->settings.column_ordering[index];
    if (state->settings.grouped_column == j) {
        return false;
    }
    return state->settings.column_enabled[j];
}
void TableItemArrangerModel::set_enabled(int index, bool enable) {
    state->settings_changed = true;
    auto j = state->settings.column_ordering[index];
    if (state->settings.grouped_column == j) {
        state->settings.grouped_column = -1;
        state->settings.group_collapsed.clear();
    }
    state->settings.column_enabled[j] = enable;
    refresh_results(state);
}
void TableItemArrangerModel::reorder(int old_index, int new_index) {
    state->settings_changed = true;
    auto& settings = state->settings;
    if (old_index < new_index - 1) {
        int tmp = settings.column_ordering[old_index];
        for (int i = old_index; i < new_index - 1; ++i) {
            settings.column_ordering[i] = settings.column_ordering[i + 1];
        }
        settings.column_ordering[new_index - 1] = tmp;
    }
    if (old_index > new_index) {
        int tmp = settings.column_ordering[old_index];
        for (int i = old_index; i > new_index; --i) {
            settings.column_ordering[i] = settings.column_ordering[i - 1];
        }
        settings.column_ordering[new_index] = tmp;
    }
    refresh_results(state);
}

}
