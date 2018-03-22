//
//  view.cpp
//  ddui-table
//
//  Created by Bartholomew Joyce on 22/03/2018.
//  Copyright © 2018 Bartholomew Joyce All rights reserved.
//

#include "view.hpp"
#include "style.hpp"
#include <ddui/util/draw_text_in_box>
#include <ddui/views/ContextMenu>

namespace Table {

static void refresh_model(TableState* state);
static int calculate_table_width(TableState* table_state);
static void update_function_bar(TableState* state, Context ctx, int* bar_height);
static void update_table_content(TableState* state, Context ctx);
static void update_table_headers(TableState* state, Context ctx);
static void update_column_separators(TableState* state, Context ctx);

TableState::TableState() {

    source = NULL;
    private_copy_version_count = -1;

    column_resizing.active_column = -1;
    header_dragging.potential_column = -1;
    header_dragging.active_column = -1;

    content_width = 1;
    content_height = style::CELL_HEIGHT;
  
    item_arranger_model.state = this;

    item_arranger_state.model = &item_arranger_model;
    item_arranger_state.font_face = "regular";
    item_arranger_state.text_size = 14;
    item_arranger_state.color_background_enabled = style::COLOR_TEXT_HEADER;
    item_arranger_state.color_text_enabled = nvgRGB(0, 0, 0);
    item_arranger_state.color_background_vacant = nvgRGBAf(1, 1, 1, 0.3);

}

int calculate_table_width(TableState* state) {
    int width = style::SEPARATOR_WIDTH * state->headers.size();

    for (int j = 0; j < state->headers.size(); ++j) {
        if (state->enabled_columns[j]) {
            width += state->column_widths[j];
        }
    }

    return width;
}

void update(TableState* state, Context ctx) {

    refresh_model(state);
  
    // Process context menu
    {
        int toggle = ContextMenu::process_action(ctx, (void*)state);
        if (toggle == 0) {
            state->show_column_manager = !state->show_column_manager;
            *ctx.must_repaint = true;
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
        int x = ctx.mouse->x - ctx.x;
        int y = ctx.mouse->y - ctx.y;
        ContextMenu::show(ctx, (void*)state, x, y, std::move(items));
    }
  
    int function_bar_height;
    update_function_bar(state, ctx, &function_bar_height);
  
    auto child_ctx = child_context(ctx, 0, function_bar_height, ctx.width, ctx.height - function_bar_height);
    ScrollArea::update(&state->scroll_area_state, child_ctx,
                       state->content_width, state->content_height, [&](Context ctx) {
        update_table_content(state, ctx);
        update_table_headers(state, ctx);
        update_column_separators(state, ctx);
    });
    nvgRestore(ctx.vg);

}

void update_function_bar(TableState* state, Context ctx, int* bar_height) {

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

void update_table_content(TableState* state, Context ctx) {
    auto& rows = state->rows;
    auto& headers = state->headers;
    auto& enabled_columns = state->enabled_columns;
    auto& column_widths = state->column_widths;

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
    
        int x = 0;
        for (int j : state->column_ordering) {
            if (!enabled_columns[j]) continue;

            for (int i = 0; i < rows.size(); ++i) {
                int y = (i + 1) * style::CELL_HEIGHT;
                draw_text_in_box(ctx.vg, x, y, column_widths[j], style::CELL_HEIGHT, rows[i][j].c_str());
            }
            x += column_widths[j] + style::SEPARATOR_WIDTH;
        }
    }
}

void update_table_headers(TableState* state, Context ctx) {
    auto& headers = state->headers;
    auto& enabled_columns = state->enabled_columns;
    auto& column_widths = state->column_widths;
    auto& header_dragging = state->header_dragging;

    int W = ctx.width;
  
    // Header background
    int header_y = state->scroll_area_state.scroll_y;
    nvgBeginPath(ctx.vg);
    nvgFillColor(ctx.vg, style::COLOR_BG_HEADER);
    nvgRect(ctx.vg, 0, header_y, W, style::CELL_HEIGHT);
    nvgFill(ctx.vg);
  
    // Header labels
    {
        nvgFillColor(ctx.vg, style::COLOR_TEXT_HEADER);
        nvgFontFace(ctx.vg, "bold");
        nvgFontSize(ctx.vg, style::TEXT_SIZE_HEADER);
      
        int x = 0;
        for (int j : state->column_ordering) {
            if (!enabled_columns[j]) continue;

            draw_text_in_box(ctx.vg, x, header_y, column_widths[j], style::CELL_HEIGHT, headers[j].c_str());
            x += column_widths[j] + style::SEPARATOR_WIDTH;
        }
    }

    // Header dragging
//    if (header_dragging.active_column != -1) {
//        int x = header_dragging.initial_x + (ctx.mouse->x - ctx.mouse->initial_x);
//        int y = (ctx.mouse->y - ctx.mouse->initial_y);
//        int width = column_widths[header_dragging.active_column];
//
//        nvgBeginPath(ctx.vg);
//        nvgFillColor(ctx.vg, style::COLOR_BG_HEADER);
//        nvgRect(ctx.vg, x, y, width, style::CELL_HEIGHT);
//        nvgFill(ctx.vg);
//
//        nvgFillColor(ctx.vg, style::COLOR_TEXT_HEADER);
//        nvgFontFace(ctx.vg, "bold");
//        nvgFontSize(ctx.vg, style::TEXT_SIZE_HEADER);
//
//        draw_text_in_box(ctx.vg, x, y, width, style::CELL_HEIGHT, headers[header_dragging.active_column].c_str());
//    }
}

void update_column_separators(TableState* state, Context ctx) {
    auto& headers = state->headers;
    auto& enabled_columns = state->enabled_columns;
    auto& column_widths = state->column_widths;
    auto& column_resizing = state->column_resizing;

    int H = ctx.height;

    // Column separators dragging
    int separator_y = style::CELL_HEIGHT + state->scroll_area_state.scroll_y;
    int separator_height = H - separator_y;
    if (column_resizing.active_column == -1) {
        int x = style::SEPARATOR_WIDTH / 2;
        column_resizing.active_column = -1;
        for (int j : state->column_ordering) {
            if (!enabled_columns[j]) continue;

            x += column_widths[j];
            if (mouse_over(ctx, x - 6, separator_y, 12, separator_height)) {
                *ctx.cursor = CURSOR_HORIZONTAL_RESIZE;
            }
            if (mouse_hit(ctx, x - 6, separator_y, 12, separator_height)) {
                ctx.mouse->accepted = true;
                *ctx.cursor = CURSOR_HORIZONTAL_RESIZE;
                column_resizing.active_column = j;
                column_resizing.initial_width = column_widths[column_resizing.active_column];
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
            column_widths[column_resizing.active_column] = new_width;
        } else {
            column_resizing.active_column = -1;
            state->content_width = calculate_table_width(state);
        }
    }

    // Column separators
    {
        int x = 0;
        for (int j : state->column_ordering) {
            if (!enabled_columns[j]) continue;

            x += column_widths[j];
            nvgBeginPath(ctx.vg);
            nvgFillColor(ctx.vg, j == column_resizing.active_column ? style::COLOR_SEPARATOR_ACTIVE : style::COLOR_SEPARATOR);
            nvgRect(ctx.vg, x, separator_y, style::SEPARATOR_WIDTH, separator_height);
            nvgFill(ctx.vg);
            x += style::SEPARATOR_WIDTH;
        }
    }
}

void refresh_model(TableState* state) {
    if (state->source == NULL) {
        return; // No source data to refresh
    }

    if (state->source->version_count == state->private_copy_version_count) {
        return; // Model is up-to-date
    }
  
  
    bool headers_changed = false;
    if (state->headers.size() != state->source->headers.size()) {
        headers_changed = true;
    } else {
        for (int i = 0; i < state->headers.size(); ++i) {
            if (state->headers[i] != state->source->headers[i]) {
                headers_changed = true;
                break;
            }
        }
    }

    state->headers = state->source->headers;
    state->rows = state->source->rows;

    if (headers_changed) {
        state->column_widths.clear();
        while (state->column_widths.size() < state->headers.size()) {
            state->column_widths.push_back(style::CELL_WIDTH_INITIAL);
        }

        state->enabled_columns.clear();
        while (state->enabled_columns.size() < state->headers.size()) {
            state->enabled_columns.push_back(true);
        }
      
        state->column_ordering.clear();
        while (state->column_ordering.size() < state->headers.size()) {
            state->column_ordering.push_back(state->column_ordering.size());
        }
      
        if (state->column_ordering.size() > 10) {
            state->column_ordering[1] = 4;
            state->column_ordering[4] = 1;
        }
    }

    state->content_width = calculate_table_width(state);
    state->content_height = style::CELL_HEIGHT * (state->rows.size() + 1);

    state->private_copy_version_count = state->source->version_count;
}

int TableItemArrangerModel::count() {
    return state->headers.size();
}
std::string TableItemArrangerModel::label(int index) {
    return state->headers[state->column_ordering[index]];
}
bool TableItemArrangerModel::get_enabled(int index) {
    return state->enabled_columns[state->column_ordering[index]];
}
void TableItemArrangerModel::set_enabled(int index, bool enable) {
    state->enabled_columns[state->column_ordering[index]] = enable;
}
void TableItemArrangerModel::reorder(int old_index, int new_index) {
    if (old_index < new_index - 1) {
        int tmp = state->column_ordering[old_index];
        for (int i = old_index; i < new_index - 1; ++i) {
            state->column_ordering[i] = state->column_ordering[i + 1];
        }
        state->column_ordering[new_index - 1] = tmp;
    }
    if (old_index > new_index) {
        int tmp = state->column_ordering[old_index];
        for (int i = old_index; i > new_index; --i) {
            state->column_ordering[i] = state->column_ordering[i - 1];
        }
        state->column_ordering[new_index] = tmp;
    }
}

}
