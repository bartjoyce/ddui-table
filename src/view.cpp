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
#include <ddui/util/entypo>
#include <ddui/views/ContextMenu>
#include <ddui/views/Overlay>

namespace Table {

static void refresh_model(TableState* state);
static void refresh_results(TableState* state);
static int calculate_table_width(TableState* table_state);
static void update_function_bar(TableState* state, Context ctx, int* bar_height);
static void update_table_content(TableState* state, Context ctx);
static void update_table_headers(TableState* state, Context ctx);
static void update_column_separators(TableState* state, Context ctx);
static void update_filter_overlay(TableState* state, Context ctx);
static void update_filter_buttons(TableState* state, Context ctx);
static void update_filter_values(TableState* state, Context ctx);
static void update_column_header(TableState* state, Context ctx, int j, int& x, int y);
static void draw_filter_overlay_path(Context ctx, int x, int y);
static std::vector<std::string> prepare_filter_value_list(TableState* state, int column);

TableState::TableState() {

    source = NULL;
    private_copy_ref = -1;

    column_resizing.active_column = -1;
    filter_overlay.active_column = -1;

    content_width = 1;
    content_height = style::CELL_HEIGHT;
  
    item_arranger_model.state = this;

    item_arranger_state.model = &item_arranger_model;
    item_arranger_state.font_face = "regular";
    item_arranger_state.text_size = 14;
    item_arranger_state.color_background_enabled = nvgRGB(76,  207, 255);
    item_arranger_state.color_text_enabled = nvgRGB(0, 0, 0);
    item_arranger_state.color_background_vacant = nvgRGBAf(1, 1, 1, 0.3);

}

int calculate_table_width(TableState* state) {
    auto& settings = state->settings;
    auto& results = state->results;

    int width = style::SEPARATOR_WIDTH * state->source->columns();

    for (int j : results.column_indices) {
        width += settings.column_widths[j];
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
    
        int x = 0;
        for (int j : results.column_indices) {
            int y = style::CELL_HEIGHT;
            for (int i : results.row_indices) {
                draw_text_in_box(ctx.vg, x, y,
                                 settings.column_widths[j], style::CELL_HEIGHT,
                                 model->cell_text(i, j).c_str());
                y += style::CELL_HEIGHT;
            }
            x += settings.column_widths[j] + style::SEPARATOR_WIDTH;
        }
    }
}

void update_table_headers(TableState* state, Context ctx) {
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

void update_column_header(TableState* state, Context ctx, int j, int& x, int y) {
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
            state->filter_overlay.y = ctx.y + style::CELL_HEIGHT - 2 * MARGIN;
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
        
        auto content = state->source->header_text(j).c_str();
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

void update_column_separators(TableState* state, Context ctx) {
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

void draw_filter_overlay_path(Context ctx, int x, int y) {
    using namespace style::filter_overlay;

    y += ARROW_HEIGHT;
    x -= BOX_WIDTH / 2;

    auto w = BOX_WIDTH;
    auto h = BOX_HEIGHT;
    auto aw = ARROW_WIDTH;
    auto ah = ARROW_HEIGHT;
    auto r = BOX_BORDER_RADIUS;
    
    nvgBeginPath(ctx.vg);
    nvgMoveTo(ctx.vg, x + w / 2 - aw, y);
    nvgLineTo(ctx.vg, x + w / 2, y - ah);
    nvgLineTo(ctx.vg, x + w / 2 + aw, y);
    nvgLineTo(ctx.vg, x + w - r, y);
    nvgArcTo (ctx.vg, x + w, y, x + w, y + r, r);
    nvgLineTo(ctx.vg, x + w, y + h - r);
    nvgArcTo (ctx.vg, x + w, y + h, x + w - r, y + h, r);
    nvgLineTo(ctx.vg, x + r, y + h);
    nvgArcTo (ctx.vg, x, y + h, x, y + h - r, r);
    nvgLineTo(ctx.vg, x, y + r);
    nvgArcTo (ctx.vg, x, y, x + r, y, r);
    nvgClosePath(ctx.vg);
}

void update_filter_overlay(TableState* state, Context ctx) {
    using namespace style::filter_overlay;

    auto center_x = state->filter_overlay.x;
    auto center_y = state->filter_overlay.y;

    // Fill in box background
    draw_filter_overlay_path(ctx, center_x, center_y);
    nvgFillColor(ctx.vg, COLOR_BG_BOX);
    nvgFill(ctx.vg);

    // Draw buttons
    {
        auto child_ctx = child_context(ctx, center_x - BOX_WIDTH / 2,
                                       center_y + ARROW_HEIGHT, BOX_WIDTH, BUTTONS_AREA_HEIGHT);
        update_filter_buttons(state, child_ctx);
        nvgRestore(ctx.vg);
    }

    // Draw values
    {
        auto child_ctx = child_context(ctx, center_x - BOX_WIDTH / 2,
                                       center_y + ARROW_HEIGHT + BUTTONS_AREA_HEIGHT, BOX_WIDTH,
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

void update_filter_buttons(TableState* state, Context ctx) {
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
    auto grouped = false;
    
    if (draw_filter_button(ctx, x1, y, button_width_1, button_height,
                           FORM_LEFT_MOST, ascending, "ASC")) {
        settings.sort_column = ascending ? -1 : j;
        settings.sort_ascending = true;
        refresh_results(state);
    }

    if (draw_filter_button(ctx, x2, y, button_width_2, button_height,
                           FORM_MIDDLE, descending, "DESC")) {
        settings.sort_column = descending ? -1 : j;
        settings.sort_ascending = false;
        refresh_results(state);
    }

    if (draw_filter_button(ctx, x3, y, button_width_3, button_height,
                           FORM_RIGHT_MOST, grouped, "GROUP")) {
        // TODO: implement grouping
    }
    
}

void update_filter_values(TableState* state, Context ctx) {
    
    constexpr int VALUE_HEIGHT = 30;
    
    auto inner_height = VALUE_HEIGHT * (1 + state->filter_overlay.value_list.size());

    ScrollArea::update(&state->filter_overlay.scroll_area_state, ctx, ctx.width, inner_height, [&](Context ctx) {
    
        auto j = state->filter_overlay.active_column;
        auto& value_list = state->filter_overlay.value_list;
        auto& filter = state->settings.filters[j];
        
        int y = 0;
        
        {
            if (!filter.enabled) {
                nvgFillColor(ctx.vg, style::COLOR_TEXT_HEADER);
                nvgFontFace(ctx.vg, "bold");
            } else {
                nvgFillColor(ctx.vg, style::COLOR_TEXT_ROW);
                nvgFontFace(ctx.vg, "regular");
            }
            draw_text_in_box(ctx.vg, 5, y, ctx.width - 10, VALUE_HEIGHT, "Select all");
            y += VALUE_HEIGHT;
        }
        
        for (auto& value : value_list) {
            if (!filter.enabled || filter.allowed_values.find(value) != filter.allowed_values.end()) {
                nvgFillColor(ctx.vg, style::COLOR_TEXT_HEADER);
                nvgFontFace(ctx.vg, "bold");
            } else {
                nvgFillColor(ctx.vg, style::COLOR_TEXT_ROW);
                nvgFontFace(ctx.vg, "regular");
            }
            draw_text_in_box(ctx.vg, 5, y, ctx.width - 10, VALUE_HEIGHT, value.c_str());
            y += VALUE_HEIGHT;
        }
        
    });
}

void refresh_model(TableState* state) {
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
        
        state->headers.clear();
        for (int j = 0; j < num_cols; ++j) {
            state->headers.push_back(model->header_text(j));
        }

        settings.filters[2].enabled = true;
        settings.filters[2].allowed_values.insert(std::make_pair("1", true));
        settings.filters[2].allowed_values.insert(std::make_pair("2", true));
        settings.filters[2].allowed_values.insert(std::make_pair("3", true));
        settings.filters[2].allowed_values.insert(std::make_pair("4", true));
        settings.filters[2].allowed_values.insert(std::make_pair("other", true));
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

    state->private_copy_ref = model->ref();

    refresh_results(state);
}

void refresh_results(TableState* state) {
    auto model = state->source;
    auto& settings = state->settings;

    // (Re)apply the settings
    state->results = apply_settings(*model, settings);

    state->content_width = calculate_table_width(state);
    state->content_height = style::CELL_HEIGHT * (state->results.row_indices.size() + 1);
}

std::vector<std::string> prepare_filter_value_list(TableState* state, int column) {
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

int TableItemArrangerModel::count() {
    return state->source->columns();
}
std::string TableItemArrangerModel::label(int index) {
    return state->source->header_text(state->settings.column_ordering[index]);
}
bool TableItemArrangerModel::get_enabled(int index) {
    return state->settings.column_enabled[state->settings.column_ordering[index]];
}
void TableItemArrangerModel::set_enabled(int index, bool enable) {
    state->settings.column_enabled[state->settings.column_ordering[index]] = enable;
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
