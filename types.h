/*
//  Copyright (c) 2020 Peter Frane Jr. All Rights Reserved.
//
//  Use of this source code is governed by the BSD 3-Clause License that can be
//  found in the LICENSE file.
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
//  OF ANY KIND, either express or implied.
//
//  For inquiries, email the author at pfranejr AT hotmail.com
*/

#pragma once
#include <iostream>
#include <cstdio>
#include <vector>
#include <algorithm>

#include <stdexcept>
#include <sstream>
#include <iomanip> 
#include <set>
#include <map>
#include <stack>
#include <Windows.h>
#include <gdiplus.h>
#define _USE_MATH_DEFINES
#include <math.h>

using real_t = float;
using int_vector = std::vector<int32_t>;
using byte_t = uint8_t;
using byte_vector = std::vector<byte_t>;

enum class error_type
{
    none,
    file_create_error,
    file_open_failed,
    out_of_memory,
    invalid_width,
    invalid_height,
    invalid_rotation,
    missing_filename,
    invalid_parameter,
    missing_font,
    invalid_font,
    invalid_font_type,
    unsupported_font_type,
    no_current_point,
    range_check
};

#define pt_moveto 0
#define pt_lineto 1
#define pt_curveto 2
#define pt_rect 4
#define pt_closepath 128

#define NOTFOUND -1

struct pointf
{
    real_t x{ 0.0f };
    real_t y{ 0.0f };
    //bool operator==(const pointf& p2)
    //{
      //  return p2.x == x && p2.y == y;
    //}
};


struct object_record;
class object_list;
struct font_record;
class font_manager;
class page_resources;
class docpdf;
class font_manager;
struct error_message;
struct rgb_color;
struct cmyk_color;
struct solid_color;

#pragma comment(lib, "gdiplus")
