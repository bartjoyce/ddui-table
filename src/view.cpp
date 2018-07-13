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
#include <ddui/keyboard>
#include <ddui/views/ContextMenu>
#include <ddui/views/Overlay>

namespace Table {

static void refresh_model(State* state);
void refresh_results(State* state);
static int calculate_table_width(State* table_state);
static void update_function_bar(State* state, Context ctx, int* bar_height);
static void update_table_content(State* state, Context ctx);
static void update_column_separators(State* state, Context ctx);
static void update_group_headings(State* state, Context ctx);
static void update_table_headers(State* state, Context ctx);
static void update_column_header(State* state, Context ctx, int j, int& x, int y);
static void set_selection(State* state, int i, int j);
static void clear_selection(State* state);
static void stop_editing(State* state);
static void update_editable_field(State* state, Context ctx);

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
    item_arranger_state.color_background_enabled = nvgRGB(76,  207, 255);
    item_arranger_state.color_text_enabled = nvgRGB(0, 0, 0);
    item_arranger_state.color_background_vacant = nvgRGBAf(1, 1, 1, 0.3);
    
    editable_field.is_open = false;
    editable_field.is_waiting_for_second_click = false;
    editable_field.state.model = &editable_field.model;
    editable_field.state.border_radius = 0;
    editable_field.state.margin = 6;
    editable_field.model.regular_font = "regular";
    TextEdit::set_style(&editable_field.model, false, 14, nvgRGB(0x00, 0x00, 0x00));

}

int calculate_table_width(State* state) {
    auto& settings = state->settings;
    auto& results = state->results;

    int width = style::SEPARATOR_WIDTH * results.column_indices.size();

    for (int j : results.column_indices) {
        width += settings.column_widths[j];
    }

    return width;
}

void update(State* state, Context ctx) {
  
    refresh_model(state);
  
    // Process context menu
    {
        int toggle = ContextMenu::process_action(ctx, (void*)state);
        if (toggle == 0) {
            state->show_column_manager = !state->show_column_manager;
            *ctx.must_repaint = true;
        }
        if (toggle == 1) {
            state->settings.grouped_column = -1;
            state->settings.group_collapsed.clear();
            refresh_results(state);
        }
    }
    if (mouse_hit_secondary(ctx, 0, 0, ctx.width, ctx.height)) {
        std::vector<ContextMenu::Item> items;
        {
            ContextMenu::Item item;
            item.label = "Manage Columns";
            item.checked = state->show_column_manager;
            items.push_back(item);
        }
        if (state->settings.grouped_column != -1) {
            ContextMenu::Item item;
            item.label = "Reset Grouping";
            item.checked = false;
            items.push_back(item);
        }
        int x = ctx.mouse->x - ctx.x;
        int y = ctx.mouse->y - ctx.y;
        ContextMenu::show(ctx, (void*)state, x, y, std::move(items));
    }

    // Handle filter overlay
    Overlay::handle_overlay((void*)state, [state](Context ctx) {
        update_filter_overlay(state, ctx);
    });
    if (!Overlay::is_open((void*)state)) {
        state->filter_overlay.active_column = -1;
    }

    // Update function bar
    int function_bar_height;
    update_function_bar(state, ctx, &function_bar_height);

    // Update content
    auto child_ctx = child_context(ctx, 0, function_bar_height, ctx.width, ctx.height - function_bar_height);
    ScrollArea::update(&state->scroll_area_state, child_ctx,
                       state->content_width, state->content_height, [&](Context ctx) {
        update_table_content(state, ctx);
        update_column_separators(state, ctx);
        update_group_headings(state, ctx);
        update_editable_field(state, ctx);
        update_table_headers(state, ctx);
    });
    nvgRestore(ctx.vg);

    // Handle selection change
    if (state->selection.candidate_row != -1 && !ctx.mouse->accepted) {
        ctx.mouse->accepted = true;
        set_selection(state, state->selection.candidate_row,
                             state->selection.candidate_column);
        refresh_results(state);
        *ctx.must_repaint = true;
    }
    if (state->selection.row != -1 && mouse_hit(ctx, 0, 0, ctx.width, ctx.height)) {
        ctx.mouse->accepted = true;
        clear_selection(state);
        refresh_results(state);
        *ctx.must_repaint = true;
    }

}

void update_function_bar(State* state, Context ctx, int* bar_height) {

    int y = 0;
  
    if (state->show_column_manager) {
        constexpr int MARGIN = 8;
    
        nvgBeginPath(ctx.vg);
        nvgFillColor(ctx.vg, style::COLOR_BG_HEADER);
        nvgRect(ctx.vg, 0, y, ctx.width, state->item_arranger_state.content_height + 2 * MARGIN);
        nvgFill(ctx.vg);
      
        auto child_ctx = child_context(ctx, MARGIN, y + MARGIN, ctx.width - 2 * MARGIN, ctx.height);
        ItemArranger::update(&state->item_arranger_state, child_ctx);
        nvgRestore(ctx.vg);
      
        y += state->item_arranger_state.content_height + 2 * MARGIN;
    }

    *bar_height = y;
  
}

void update_table_content(State* state, Context ctx) {
    auto model = state->source;
    auto& settings = state->settings;
    auto& results = state->results;

    int W = ctx.width;
    int H = ctx.height;

    // Fill background
    nvgBeginPath(ctx.vg);
    nvgFillColor(ctx.vg, style::COLOR_BG_ROW_EVEN);
    nvgRect(ctx.vg, 0, 0, W, H);
    nvgFill(ctx.vg);

    // Odd row backgrounds
    nvgFillColor(ctx.vg, style::COLOR_BG_ROW_ODD);
    for (int y = style::CELL_HEIGHT * 2; y < H + style::CELL_HEIGHT; y += style::CELL_HEIGHT * 2) {
        nvgBeginPath(ctx.vg);
        nvgRect(ctx.vg, 0, y, W, style::CELL_HEIGHT);
        nvgFill(ctx.vg);
    }

    // Cells
    {
        nvgFillColor(ctx.vg, style::COLOR_TEXT_ROW);
        nvgFontFace(ctx.vg, "medium");
        nvgFontSize(ctx.vg, style::TEXT_SIZE_ROW);
    
        int sel_i = state->selection.row;
        int sel_j = state->selection.column;

        state->selection.candidate_row = -1;
        state->selection.candidate_column = -1;

        int x = 0;
        for (int j : results.column_indices) {
            int y = style::CELL_HEIGHT;
            for (int i : results.row_indices) {
                if (i == -1) {
                    // This row is a group heading
                    y += style::CELL_HEIGHT;
                    continue;
                }
                if (sel_i == i && sel_j == j) {
                    nvgFillColor(ctx.vg, style::COLOR_BG_CELL_ACTIVE);
                    nvgBeginPath(ctx.vg);
                    nvgRect(ctx.vg, x, y, settings.column_widths[j], style::CELL_HEIGHT);
                    nvgFill(ctx.vg);
                    nvgFillColor(ctx.vg, style::COLOR_TEXT_ROW);
                    
                    state->editable_field.cell_x = x;
                    state->editable_field.cell_y = y;
                    state->editable_field.cell_width = settings.column_widths[j];
                }
                if (mouse_hit(ctx, x, y, settings.column_widths[j], style::CELL_HEIGHT)) {
                    state->selection.candidate_row = i;
                    state->selection.candidate_column = j;
                }
                draw_centered_text_in_box(ctx.vg, x, y,
                                          settings.column_widths[j], style::CELL_HEIGHT,
                                          model->cell_text(i, j).c_str());
                y += style::CELL_HEIGHT;
            }
            x += settings.column_widths[j] + style::SEPARATOR_WIDTH;
        }
    }
}

void update_table_headers(State* state, Context ctx) {
    auto& results = state->results;

    // Header background
    int header_y = state->scroll_area_state.scroll_y;
    nvgBeginPath(ctx.vg);
    nvgFillColor(ctx.vg, style::COLOR_BG_HEADER);
    nvgRect(ctx.vg, 0, header_y, ctx.width, style::CELL_HEIGHT);
    nvgFill(ctx.vg);

    // Header columns
    int x = 0;
    for (int j : results.column_indices) {
        update_column_header(state, ctx, j, x, header_y);
    }
}

void update_column_header(State* state, Context ctx, int j, int& x, int y) {
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

        constexpr auto MARGIN = 2;

        nvgFontFace(ctx.vg, "entypo");
        nvgFontSize(ctx.vg, 24);

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
        nvgTextMetrics(ctx.vg, &ascender, &descender, &line_height);
        
        int text_y = y + (style::CELL_HEIGHT - (int)line_height) / 2 + (int)ascender;
        
        float bounds[4];
        nvgTextBounds(ctx.vg, 0, 0, icon_text, 0, bounds);
        icon_size = bounds[2] - bounds[0] + 2 * MARGIN;
        
        if (settings.filters[j].enabled || settings.sort_column == j) {
            nvgFillColor(ctx.vg, style::COLOR_TEXT_HEADER);
        } else {
            nvgFillColor(ctx.vg, style::COLOR_BG_ROW_ODD);
        }
    
        if (mouse_over(ctx, x + settings.column_widths[j] - icon_size, y, icon_size, style::CELL_HEIGHT)) {
            *ctx.cursor = CURSOR_POINTING_HAND;
        }

        if (mouse_hit(ctx, x + settings.column_widths[j] - icon_size, y, icon_size, style::CELL_HEIGHT)) {
            ctx.mouse->accepted = true;
            state->filter_overlay.active_column = j;
            state->filter_overlay.x = ctx.x + x + settings.column_widths[j] - icon_size / 2;
            state->filter_overlay.y = ctx.y + state->scroll_area_state.scroll_y + style::CELL_HEIGHT - 2 * MARGIN;
            state->filter_overlay.value_list = prepare_filter_value_list(state, j);
            state->filter_overlay.scroll_area_state = ScrollArea::ScrollAreaState();
            Overlay::open((void*)state);
        }
        
        if (state->filter_overlay.active_column == j) {
            nvgFillColor(ctx.vg, style::COLOR_TEXT_ROW);
        }
        
        nvgText(ctx.vg, x + settings.column_widths[j] - icon_size + MARGIN, text_y, icon_text, NULL);
    }

    // Header labels
    {
        nvgFillColor(ctx.vg, style::COLOR_TEXT_HEADER);
        nvgFontFace(ctx.vg, "bold");
        nvgFontSize(ctx.vg, style::TEXT_SIZE_HEADER);

        float ascender, descender, line_height;
        nvgTextMetrics(ctx.vg, &ascender, &descender, &line_height);

        int text_y = y + (style::CELL_HEIGHT - (int)line_height) / 2 + (int)ascender;
        
        constexpr int MARGIN = 2;
        
        auto content_std = state->source->header_text(j);
        auto content = content_std.c_str();
        auto width = settings.column_widths[j];
      
        auto length = strlen(content);
        char new_content[length + 4];
        auto text_width = truncate_text(ctx.vg, width - 2 * MARGIN - icon_size, length, new_content, content);
      
        auto text_x = (width - text_width) / 2 + MARGIN;
        if (width - (text_x + text_width) < icon_size) {
            text_x = width - (icon_size + text_width);
        }

        nvgText(ctx.vg, x + text_x, text_y, new_content, 0);
    }
    
    x += settings.column_widths[j] + style::SEPARATOR_WIDTH;
}

void update_column_separators(State* state, Context ctx) {
    auto& settings = state->settings;
    auto& results = state->results;
    auto& column_resizing = state->column_resizing;

    int H = ctx.height;

    // Column separators dragging
    int separator_y = style::CELL_HEIGHT + state->scroll_area_state.scroll_y;
    int separator_height = H - separator_y;
    if (column_resizing.active_column == -1) {
        int x = style::SEPARATOR_WIDTH / 2;
        column_resizing.active_column = -1;
        for (int j : results.column_indices) {
            x += settings.column_widths[j];
            if (mouse_over(ctx, x - 6, separator_y, 12, separator_height)) {
                *ctx.cursor = CURSOR_HORIZONTAL_RESIZE;
            }
            if (mouse_hit(ctx, x - 6, separator_y, 12, separator_height)) {
                ctx.mouse->accepted = true;
                *ctx.cursor = CURSOR_HORIZONTAL_RESIZE;
                column_resizing.active_column = j;
                column_resizing.initial_width = settings.column_widths[column_resizing.active_column];
                break;
            }
            x += style::SEPARATOR_WIDTH;
        }
    } else {
        if (ctx.mouse->pressed) {
            *ctx.cursor = CURSOR_HORIZONTAL_RESIZE;
            int dx = ctx.mouse->x - ctx.mouse->initial_x;
            int new_width = column_resizing.initial_width + dx;
            new_width = new_width > 50 ? new_width : 50;
            settings.column_widths[column_resizing.active_column] = new_width;
        } else {
            column_resizing.active_column = -1;
            state->content_width = calculate_table_width(state);
        }
    }

    // Column separators
    {
        int x = 0;
        for (int j : results.column_indices) {
            x += settings.column_widths[j];
            nvgBeginPath(ctx.vg);
            nvgFillColor(ctx.vg, j == column_resizing.active_column ? style::COLOR_SEPARATOR_ACTIVE : style::COLOR_SEPARATOR);
            nvgRect(ctx.vg, x, separator_y, style::SEPARATOR_WIDTH, separator_height);
            nvgFill(ctx.vg);
            x += style::SEPARATOR_WIDTH;
        }
    }
}

void update_group_headings(State* state, Context ctx) {
    if (state->settings.grouped_column == -1) {
        return;
    }

    constexpr int BUTTON_SIZE = 28;

    auto& model = *state->source;
    auto& settings = state->settings;
    auto& results = state->results;
    
    float ascender, descender, line_height;
    float bounds[4];
    
    // Measure expand/collapse button
    nvgFontFace(ctx.vg, "entypo");
    nvgFontSize(ctx.vg, BUTTON_SIZE);
    
    nvgTextMetrics(ctx.vg, &ascender, &descender, &line_height);
    nvgTextBounds(ctx.vg, 0, 0, entypo::BLACK_DOWNPOINTING_SMALL_TRIANGLE, NULL, bounds);
    int x = state->scroll_area_state.scroll_x;
    float button_x = x + style::GROUP_HEADING_MARGIN;
    float button_y = (style::CELL_HEIGHT - line_height) / 2 + ascender;
    float button_width = bounds[2] - bounds[0];
    
    // Prepare & measure text
    auto header_text = model.header_text(settings.grouped_column);
    char buffer1[header_text.size() + 10];
    char buffer2[10];
    sprintf(buffer1, "  %s:  ", header_text.c_str());
    
    nvgFontFace(ctx.vg, "regular");
    nvgFontSize(ctx.vg, style::TEXT_SIZE_GROUP_HEADING);
    nvgTextMetrics(ctx.vg, &ascender, &descender, &line_height);
    nvgTextBounds(ctx.vg, 0, 0, buffer1, NULL, bounds);
    float text_y = (style::CELL_HEIGHT - line_height) / 2 + ascender;
    float column_text_x = button_x + button_width;
    float column_text_width = bounds[2] - bounds[0];
    
    for (auto& heading : results.group_headings) {
        auto collapsed = settings.group_collapsed[heading.value];
        int y = (1 + heading.position) * style::CELL_HEIGHT;
        int width = ctx.clip.x2 - ctx.clip.x1;
        
        // Fill background
        nvgFillColor(ctx.vg, style::COLOR_BG_GROUP_HEADING);
        nvgBeginPath(ctx.vg);
        nvgRect(ctx.vg, x, y, width, style::CELL_HEIGHT);
        nvgFill(ctx.vg);
        
        // Border line
        nvgStrokeColor(ctx.vg, style::COLOR_SEPARATOR);
        nvgStrokeWidth(ctx.vg, 1.0);
        nvgBeginPath(ctx.vg);
        nvgMoveTo(ctx.vg, x, y);
        nvgLineTo(ctx.vg, x + width, y);
        nvgStroke(ctx.vg);
        
        // Draw expand/collapse button
        if (mouse_over(ctx, button_x - 2, y, button_width + 2, style::CELL_HEIGHT)) {
            *ctx.cursor = CURSOR_POINTING_HAND;
            nvgFillColor(ctx.vg, style::COLOR_TEXT_GROUP_HEADING_HOVER);
        } else {
            nvgFillColor(ctx.vg, style::COLOR_TEXT_GROUP_HEADING);
        }
        if (mouse_hit(ctx, button_x - 2, y, button_width + 2, style::CELL_HEIGHT)) {
            ctx.mouse->accepted = true;
            settings.group_collapsed[heading.value] = !collapsed;
            refresh_results(state);
            *ctx.must_repaint = true;
            return;
        }
        nvgFontFace(ctx.vg, "entypo");
        nvgFontSize(ctx.vg, BUTTON_SIZE);
        nvgText(ctx.vg, button_x, y + button_y,
                collapsed ? entypo::BLACK_RIGHTPOINTING_SMALL_TRIANGLE : entypo::BLACK_DOWNPOINTING_SMALL_TRIANGLE, NULL);
        
        // Draw column title
        if (mouse_over(ctx, column_text_x, y, column_text_width, style::CELL_HEIGHT)) {
            *ctx.cursor = CURSOR_POINTING_HAND;
            nvgFillColor(ctx.vg, style::COLOR_TEXT_GROUP_HEADING_HOVER);
        } else {
            nvgFillColor(ctx.vg, style::COLOR_TEXT_GROUP_HEADING);
        }
        if (mouse_hit(ctx, column_text_x, y, column_text_width, style::CELL_HEIGHT)) {
            ctx.mouse->accepted = true;
            state->filter_overlay.active_column = settings.grouped_column;
            state->filter_overlay.x = ctx.x + column_text_x + column_text_width / 2;
            state->filter_overlay.y = ctx.y + y + style::CELL_HEIGHT - 2;
            state->filter_overlay.value_list = prepare_filter_value_list(state, settings.grouped_column);
            state->filter_overlay.scroll_area_state = ScrollArea::ScrollAreaState();
            Overlay::open((void*)state);
        }
        nvgFontFace(ctx.vg, "regular");
        nvgFontSize(ctx.vg, style::TEXT_SIZE_GROUP_HEADING);
        nvgText(ctx.vg, column_text_x, y + text_y, buffer1, NULL);
        
        // Draw value text
        nvgFontFace(ctx.vg, "bold");
        nvgFillColor(ctx.vg, style::COLOR_TEXT_GROUP_HEADING);
        nvgTextBounds(ctx.vg, 0, 0, heading.value.c_str(), NULL, bounds);
        float value_text_x = column_text_x + column_text_width;
        float value_text_width = bounds[2] - bounds[0];
        nvgText(ctx.vg, value_text_x, y + text_y, heading.value.c_str(), NULL);
        
        // Draw count text
        nvgFontFace(ctx.vg, "regular");
        float count_text_x = value_text_x + value_text_width;
        sprintf(buffer2, " (%d)", heading.count);
        nvgText(ctx.vg, count_text_x, y + text_y, buffer2, NULL);
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

void set_selection(State* state, int i, int j) {
    state->selection.row = state->selection.candidate_row;
    state->selection.column = state->selection.candidate_column;
    state->selection.row_key.clear();

    auto key = state->source->key();
    if (!key.empty()) {
        auto i = state->selection.row;
        for (auto j : key) {
            state->selection.row_key.push_back(state->source->cell_text(i, j));
        }
    }

    stop_editing(state);

    state->editable_field.is_waiting_for_second_click = true;
    state->editable_field.click_time = std::chrono::high_resolution_clock::now();
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

void update_editable_field(State* state, Context ctx) {
    // Dealing with the double-click
    if (state->editable_field.is_waiting_for_second_click &&
        mouse_hit(ctx, state->editable_field.cell_x, state->editable_field.cell_y,
                  state->editable_field.cell_width, style::CELL_HEIGHT)) {

        auto current_time = std::chrono::high_resolution_clock::now();
        auto time_elapsed = std::chrono::duration_cast<std::chrono::duration<double>>
                            (current_time - state->editable_field.click_time).count();
        state->editable_field.is_waiting_for_second_click = false;

        if (time_elapsed >= 0.5) {
            return;
        }

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
        keyboard::focus(ctx, &state->editable_field.state);
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

    if (keyboard::has_key_event(ctx, &state->editable_field.state)) {
        if (ctx.key->action == keyboard::ACTION_PRESS &&
            ctx.key->key == keyboard::KEY_ENTER) {
            keyboard::consume_key_event(ctx);
        }
        if (ctx.key->action == keyboard::ACTION_RELEASE &&
            ctx.key->key == keyboard::KEY_ENTER) {
            stop_editing(state);
            keyboard::consume_key_event(ctx);
            return;
        }
    }

    auto child_ctx = child_context(ctx, state->editable_field.cell_x,
                                        state->editable_field.cell_y,
                                        state->editable_field.cell_width,
                                        style::CELL_HEIGHT);
    PlainTextBox::update(&state->editable_field.state, child_ctx);
    nvgRestore(ctx.vg);
    
    // Select all on focus
    if (keyboard::did_focus(ctx, &state->editable_field.state)) {
        auto& model = state->editable_field.model;
        model.selection = { 0 };
        model.selection.b_index = model.lines.front().characters.size();
        model.version_count++;
        *ctx.must_repaint = true;
    }
    
    // Close the box on blur
    if (keyboard::did_blur(ctx, &state->editable_field.state)) {
        stop_editing(state);
        *ctx.must_repaint = true;
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
    auto j = state->settings.column_ordering[index];
    if (state->settings.grouped_column == j) {
        state->settings.grouped_column = -1;
        state->settings.group_collapsed.clear();
    }
    state->settings.column_enabled[j] = enable;
    refresh_results(state);
}
void TableItemArrangerModel::reorder(int old_index, int new_index) {
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
