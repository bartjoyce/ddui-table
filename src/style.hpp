//
//  style.hpp
//  ddui-table
//
//  Created by Bartholomew Joyce on 22/03/2018.
//  Copyright © 2018 Bartholomew Joyce All rights reserved.
//

#ifndef ddui_table_style_hpp
#define ddui_table_style_hpp

#include <ddui/core>

namespace Table {
namespace style {

extern ddui::Color COLOR_BG_ROW_EVEN;
extern ddui::Color COLOR_BG_ROW_ODD;
extern ddui::Color COLOR_BG_CELL_ACTIVE;
extern ddui::Color COLOR_BG_HEADER;
extern ddui::Color COLOR_BG_GROUP_HEADING;
extern ddui::Color COLOR_TEXT_ROW;
extern ddui::Color COLOR_TEXT_HEADER;
extern ddui::Color COLOR_TEXT_GROUP_HEADING;
extern ddui::Color COLOR_TEXT_GROUP_HEADING_HOVER;
extern ddui::Color COLOR_SEPARATOR;
extern ddui::Color COLOR_SEPARATOR_ACTIVE;
extern float CELL_WIDTH_INITIAL;
extern float CELL_HEIGHT;
extern float SEPARATOR_WIDTH;
extern float TEXT_SIZE_ROW;
extern float TEXT_SIZE_HEADER;
extern float TEXT_SIZE_GROUP_HEADING;
extern float GROUP_HEADING_MARGIN;

// Filter overlay
namespace filter_overlay {

    extern ddui::Color COLOR_OUTLINE;
    extern ddui::Color COLOR_BG_BOX;
    extern ddui::Color COLOR_BG_BUTTON;
    extern ddui::Color COLOR_BG_BUTTON_ACTIVE;
    extern ddui::Color COLOR_TEXT_BUTTON;
    extern ddui::Color COLOR_VALUE_SEPARATOR;
    extern float ARROW_HEIGHT;
    extern float ARROW_WIDTH;
    extern float BOX_WIDTH;
    extern float BOX_HEIGHT;
    extern float BOX_BORDER_RADIUS;
    extern float BUTTONS_AREA_HEIGHT;
    extern float BUTTONS_AREA_MARGIN;
    extern float BUTTON_BORDER_RADIUS;
    extern float BUTTON_SPACING;
    extern float VALUE_HEIGHT;
    extern float VALUE_MARGIN;
    extern float VALUE_SQUARE_SIZE;
    extern float VALUE_SQUARE_MARGIN;
    extern float VALUE_SQUARE_BORDER_RADIUS;

}

}
}

#endif
