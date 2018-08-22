//
//  style.hpp
//  ddui-table
//
//  Created by Bartholomew Joyce on 22/03/2018.
//  Copyright © 2018 Bartholomew Joyce All rights reserved.
//

#ifndef ddui_table_style_hpp
#define ddui_table_style_hpp

#include <nanovg.h>

namespace Table {
namespace style {

extern NVGcolor COLOR_BG_ROW_EVEN;
extern NVGcolor COLOR_BG_ROW_ODD;
extern NVGcolor COLOR_BG_CELL_ACTIVE;
extern NVGcolor COLOR_BG_HEADER;
extern NVGcolor COLOR_BG_GROUP_HEADING;
extern NVGcolor COLOR_TEXT_ROW;
extern NVGcolor COLOR_TEXT_HEADER;
extern NVGcolor COLOR_TEXT_GROUP_HEADING;
extern NVGcolor COLOR_TEXT_GROUP_HEADING_HOVER;
extern NVGcolor COLOR_SEPARATOR;
extern NVGcolor COLOR_SEPARATOR_ACTIVE;
extern int CELL_WIDTH_INITIAL;
extern int CELL_HEIGHT;
extern int SEPARATOR_WIDTH;
extern int TEXT_SIZE_ROW;
extern int TEXT_SIZE_HEADER;
extern int TEXT_SIZE_GROUP_HEADING;
extern int GROUP_HEADING_MARGIN;

// Filter overlay
namespace filter_overlay {

    extern NVGcolor COLOR_OUTLINE;
    extern NVGcolor COLOR_BG_BOX;
    extern NVGcolor COLOR_BG_BUTTON;
    extern NVGcolor COLOR_BG_BUTTON_ACTIVE;
    extern NVGcolor COLOR_TEXT_BUTTON;
    extern NVGcolor COLOR_VALUE_SEPARATOR;
    extern int ARROW_HEIGHT;
    extern int ARROW_WIDTH;
    extern int BOX_WIDTH;
    extern int BOX_HEIGHT;
    extern int BOX_BORDER_RADIUS;
    extern int BUTTONS_AREA_HEIGHT;
    extern int BUTTONS_AREA_MARGIN;
    extern int BUTTON_BORDER_RADIUS;
    extern int BUTTON_SPACING;
    extern int VALUE_HEIGHT;
    extern int VALUE_MARGIN;
    extern int VALUE_SQUARE_SIZE;
    extern int VALUE_SQUARE_MARGIN;
    extern int VALUE_SQUARE_BORDER_RADIUS;

}

}
}

#endif
