#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "mtx/render_ir.hpp"
#include <windows.h>

namespace mtx {

void display_list_to_gdi(HDC hdc, const DisplayList& list, int origin_x, int origin_y);

} // namespace mtx
