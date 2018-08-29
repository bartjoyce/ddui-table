//
//  style.cpp
//  ddui-table
//
//  Created by Bartholomew Joyce on 22/08/2018.
//  Copyright © 2018 Bartholomew Joyce All rights reserved.
//

#include "style.hpp"

namespace Table {
namespace style {

ddui::Color COLOR_BG_ROW_EVEN              = ddui::rgb(0x424242);
ddui::Color COLOR_BG_ROW_ODD               = ddui::rgb(0x4f4f4f);
ddui::Color COLOR_BG_CELL_ACTIVE           = ddui::rgb(0x2a9fd6);
ddui::Color COLOR_BG_HEADER                = ddui::rgb(0x222222);
ddui::Color COLOR_BG_GROUP_HEADING         = ddui::rgb(0x777777);
ddui::Color COLOR_TEXT_ROW                 = ddui::rgb(0xffffff);
ddui::Color COLOR_TEXT_HEADER              = ddui::rgb(0x4ccfff);
ddui::Color COLOR_TEXT_GROUP_HEADING       = ddui::rgb(0xdddddd);
ddui::Color COLOR_TEXT_GROUP_HEADING_HOVER = ddui::rgb(0xffffff);
ddui::Color COLOR_SEPARATOR                = ddui::rgba(0xbbbbbb, 0.125);
ddui::Color COLOR_SEPARATOR_ACTIVE         = ddui::rgb(0x2a9fd6);
float CELL_WIDTH_INITIAL = 100;
float CELL_HEIGHT = 24;
float SEPARATOR_WIDTH = 1;
float TEXT_SIZE_ROW = 14;
float TEXT_SIZE_HEADER = 14;
float TEXT_SIZE_GROUP_HEADING = 14;
float GROUP_HEADING_MARGIN = 10;

// Filter overlay
namespace filter_overlay {

    ddui::Color COLOR_OUTLINE          = COLOR_SEPARATOR;
    ddui::Color COLOR_BG_BOX           = COLOR_BG_HEADER;
    ddui::Color COLOR_BG_BUTTON        = COLOR_BG_ROW_EVEN;
    ddui::Color COLOR_BG_BUTTON_ACTIVE = COLOR_SEPARATOR_ACTIVE;
    ddui::Color COLOR_TEXT_BUTTON      = COLOR_TEXT_ROW;
    ddui::Color COLOR_VALUE_SEPARATOR  = COLOR_BG_ROW_ODD;
    float ARROW_HEIGHT = 10;
    float ARROW_WIDTH = 6;
    float BOX_WIDTH = 200;
    float BOX_HEIGHT = 200;
    float BOX_BORDER_RADIUS = 4;
    float BUTTONS_AREA_HEIGHT = 40;
    float BUTTONS_AREA_MARGIN = 6;
    float BUTTON_BORDER_RADIUS = 4;
    float BUTTON_SPACING = 1;
    float VALUE_HEIGHT = 30;
    float VALUE_MARGIN = 5;
    float VALUE_SQUARE_SIZE = 10;
    float VALUE_SQUARE_MARGIN = 10;
    float VALUE_SQUARE_BORDER_RADIUS = 2;

}

}
}
