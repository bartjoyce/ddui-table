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

namespace style {

static NVGcolor COLOR_BG_ROW_EVEN              = nvgRGB(0x42, 0x42, 0x42); // #424242
static NVGcolor COLOR_BG_ROW_ODD               = nvgRGB(0x4f, 0x4f, 0x4f); // #4f4f4f
static NVGcolor COLOR_BG_HEADER                = nvgRGB(0x22, 0x22, 0x22); // #222222
static NVGcolor COLOR_BG_GROUP_HEADING         = nvgRGB(0x77, 0x77, 0x77); // #777777
static NVGcolor COLOR_TEXT_ROW                 = nvgRGB(0xff, 0xff, 0xff); // #ffffff
static NVGcolor COLOR_TEXT_HEADER              = nvgRGB(0x4c, 0xcf, 0xff); // #4ccfff
static NVGcolor COLOR_TEXT_GROUP_HEADING       = nvgRGB(0xdd, 0xdd, 0xdd); // #dddddd
static NVGcolor COLOR_TEXT_GROUP_HEADING_HOVER = nvgRGB(0xff, 0xff, 0xff); // #ffffff
static NVGcolor COLOR_SEPARATOR                = nvgRGB(0xbb, 0xbb, 0xbb); // #bbbbbb
static NVGcolor COLOR_SEPARATOR_ACTIVE         = nvgRGB(0x2a, 0x9f, 0xd6); // #2a9fd6
constexpr int CELL_WIDTH_INITIAL = 100;
constexpr int CELL_HEIGHT = 25;
constexpr int SEPARATOR_WIDTH = 2;
constexpr int TEXT_SIZE_ROW = 14;
constexpr int TEXT_SIZE_HEADER = 14;
constexpr int TEXT_SIZE_GROUP_HEADING = 14;
constexpr int GROUP_HEADING_MARGIN = 10;

// Filter overlay
namespace filter_overlay {

    static NVGcolor COLOR_OUTLINE          = COLOR_SEPARATOR;
    static NVGcolor COLOR_BG_BOX           = COLOR_BG_HEADER;
    static NVGcolor COLOR_BG_BUTTON        = COLOR_BG_ROW_EVEN;
    static NVGcolor COLOR_BG_BUTTON_ACTIVE = COLOR_SEPARATOR_ACTIVE;
    static NVGcolor COLOR_TEXT_BUTTON      = COLOR_TEXT_ROW;
    static NVGcolor COLOR_VALUE_SEPARATOR  = COLOR_BG_ROW_ODD;
    constexpr int ARROW_HEIGHT = 10;
    constexpr int ARROW_WIDTH = 6;
    constexpr int BOX_WIDTH = 200;
    constexpr int BOX_HEIGHT = 200;
    constexpr int BOX_BORDER_RADIUS = 4;
    constexpr int BUTTONS_AREA_HEIGHT = 40;
    constexpr int BUTTONS_AREA_MARGIN = 6;
    constexpr int BUTTON_BORDER_RADIUS = 4;
    constexpr int BUTTON_SPACING = 1;
    constexpr int VALUE_HEIGHT = 30;
    constexpr int VALUE_MARGIN = 5;
    constexpr int VALUE_SQUARE_SIZE = 10;
    constexpr int VALUE_SQUARE_MARGIN = 10;
    constexpr int VALUE_SQUARE_BORDER_RADIUS = 2;

}

}

#endif
