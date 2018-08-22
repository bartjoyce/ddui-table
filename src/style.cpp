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

NVGcolor COLOR_BG_ROW_EVEN              = nvgRGB(0x42, 0x42, 0x42); // #424242
NVGcolor COLOR_BG_ROW_ODD               = nvgRGB(0x4f, 0x4f, 0x4f); // #4f4f4f
NVGcolor COLOR_BG_CELL_ACTIVE           = nvgRGB(0x2a, 0x9f, 0xd6); // #2a9fd6
NVGcolor COLOR_BG_HEADER                = nvgRGB(0x22, 0x22, 0x22); // #222222
NVGcolor COLOR_BG_GROUP_HEADING         = nvgRGB(0x77, 0x77, 0x77); // #777777
NVGcolor COLOR_TEXT_ROW                 = nvgRGB(0xff, 0xff, 0xff); // #ffffff
NVGcolor COLOR_TEXT_HEADER              = nvgRGB(0x4c, 0xcf, 0xff); // #4ccfff
NVGcolor COLOR_TEXT_GROUP_HEADING       = nvgRGB(0xdd, 0xdd, 0xdd); // #dddddd
NVGcolor COLOR_TEXT_GROUP_HEADING_HOVER = nvgRGB(0xff, 0xff, 0xff); // #ffffff
NVGcolor COLOR_SEPARATOR                = nvgRGBA(0xbb, 0xbb, 0xbb, 0x20); // #bbbbbb
NVGcolor COLOR_SEPARATOR_ACTIVE         = nvgRGB(0x2a, 0x9f, 0xd6); // #2a9fd6
int CELL_WIDTH_INITIAL = 100;
int CELL_HEIGHT = 24;
int SEPARATOR_WIDTH = 1;
int TEXT_SIZE_ROW = 14;
int TEXT_SIZE_HEADER = 14;
int TEXT_SIZE_GROUP_HEADING = 14;
int GROUP_HEADING_MARGIN = 10;

// Filter overlay
namespace filter_overlay {

    NVGcolor COLOR_OUTLINE          = COLOR_SEPARATOR;
    NVGcolor COLOR_BG_BOX           = COLOR_BG_HEADER;
    NVGcolor COLOR_BG_BUTTON        = COLOR_BG_ROW_EVEN;
    NVGcolor COLOR_BG_BUTTON_ACTIVE = COLOR_SEPARATOR_ACTIVE;
    NVGcolor COLOR_TEXT_BUTTON      = COLOR_TEXT_ROW;
    NVGcolor COLOR_VALUE_SEPARATOR  = COLOR_BG_ROW_ODD;
    int ARROW_HEIGHT = 10;
    int ARROW_WIDTH = 6;
    int BOX_WIDTH = 200;
    int BOX_HEIGHT = 200;
    int BOX_BORDER_RADIUS = 4;
    int BUTTONS_AREA_HEIGHT = 40;
    int BUTTONS_AREA_MARGIN = 6;
    int BUTTON_BORDER_RADIUS = 4;
    int BUTTON_SPACING = 1;
    int VALUE_HEIGHT = 30;
    int VALUE_MARGIN = 5;
    int VALUE_SQUARE_SIZE = 10;
    int VALUE_SQUARE_MARGIN = 10;
    int VALUE_SQUARE_BORDER_RADIUS = 2;

}

}
}
