#include "mtx/render_gdi_win32.hpp"
#include <algorithm>
#include <string>

// GDI+ headers are not self-contained under WIN32_LEAN_AND_MEAN.
// The MDraft Win32 build intentionally uses WIN32_LEAN_AND_MEAN,
// therefore the COM/GUID substrate required by gdiplus.h must be
// pulled in explicitly before gdiplus.h.
#include <guiddef.h>
#include <unknwn.h>
#include <objidl.h>
#include <propidl.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

namespace mtx {
namespace {

struct GdiplusRuntime {
  GdiplusRuntime() {
    Gdiplus::GdiplusStartupInput input;
    Gdiplus::GdiplusStartup(&token, &input, nullptr);
  }
  ~GdiplusRuntime() {
    if (token != 0) Gdiplus::GdiplusShutdown(token);
  }
  ULONG_PTR token{0};
};

GdiplusRuntime& gdiplus_runtime() {
  static GdiplusRuntime rt;
  return rt;
}


COLORREF colorref(Rgba c) {
  return RGB(c.r, c.g, c.b);
}

std::wstring widen_utf8(const std::string& s) {
  if (s.empty()) return {};
  int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
  if (n <= 0) {
    std::wstring out;
    out.reserve(s.size());
    for (unsigned char c : s) out.push_back(static_cast<wchar_t>(c));
    return out;
  }
  std::wstring out(static_cast<std::size_t>(n), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n);
  return out;
}

void fill_rect_gdi(HDC hdc, int x, int y, int w, int h, Rgba c) {
  RECT r{x, y, x + std::max(0, w), y + std::max(0, h)};
  HBRUSH br = CreateSolidBrush(colorref(c));
  FillRect(hdc, &r, br);
  DeleteObject(br);
}

const wchar_t* gdi_font_family(TextFace face) noexcept {
  switch (face) {
    case TextFace::Sans:
      return L"Segoe UI";
    case TextFace::Monospace:
    default:
      return L"Consolas";
  }
}

} // namespace

void display_list_to_gdi(HDC hdc, const DisplayList& list, int origin_x, int origin_y) {
  for (const auto& c : list.commands) {
    switch (c.kind) {
      case DrawKind::Rect: {
        fill_rect_gdi(hdc, origin_x + c.x, origin_y + c.y, c.w, c.h, c.fill);
        if (c.stroke_width > 0) {
          HPEN pen = CreatePen(PS_SOLID, c.stroke_width, colorref(c.stroke));
          HGDIOBJ old_pen = SelectObject(hdc, pen);
          HGDIOBJ old_brush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
          Rectangle(hdc, origin_x + c.x, origin_y + c.y, origin_x + c.x + c.w, origin_y + c.y + c.h);
          SelectObject(hdc, old_brush);
          SelectObject(hdc, old_pen);
          DeleteObject(pen);
        }
        break;
      }
      case DrawKind::Line: {
        HPEN pen = CreatePen(PS_SOLID, std::max(1, c.stroke_width), colorref(c.stroke));
        HGDIOBJ old_pen = SelectObject(hdc, pen);
        MoveToEx(hdc, origin_x + c.x, origin_y + c.y, nullptr);
        LineTo(hdc, origin_x + c.x2, origin_y + c.y2);
        SelectObject(hdc, old_pen);
        DeleteObject(pen);
        break;
      }
      case DrawKind::Text: {
        HFONT font = CreateFontW(-c.font_size, 0, 0, 0,
                                 c.weight == TextWeight::Bold ? FW_BOLD : FW_NORMAL,
                                 FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY,
                                 c.face == TextFace::Sans ? (VARIABLE_PITCH | FF_SWISS) : (FIXED_PITCH | FF_MODERN),
                                 gdi_font_family(c.face));
        HGDIOBJ old_font = SelectObject(hdc, font);
        SetTextColor(hdc, colorref(c.fill));
        SetBkMode(hdc, TRANSPARENT);
        const std::wstring ws = widen_utf8(c.text);
        TextOutW(hdc, origin_x + c.x, origin_y + c.y, ws.c_str(), static_cast<int>(ws.size()));
        SelectObject(hdc, old_font);
        DeleteObject(font);
        break;
      }
      case DrawKind::Image: {
        fill_rect_gdi(hdc, origin_x + c.x, origin_y + c.y, c.w, c.h, c.fill);
        if (c.stroke_width > 0) {
          HPEN pen = CreatePen(PS_SOLID, c.stroke_width, colorref(c.stroke));
          HGDIOBJ old_pen = SelectObject(hdc, pen);
          HGDIOBJ old_brush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
          Rectangle(hdc, origin_x + c.x, origin_y + c.y, origin_x + c.x + c.w, origin_y + c.y + c.h);
          SelectObject(hdc, old_brush);
          SelectObject(hdc, old_pen);
          DeleteObject(pen);
        }

        (void)gdiplus_runtime();
        const std::wstring path = widen_utf8(c.path);
        bool drawn = false;
        if (!path.empty()) {
          Gdiplus::Graphics g(hdc);
          Gdiplus::Image image(path.c_str());
          if (image.GetLastStatus() == Gdiplus::Ok) {
            const UINT iw = image.GetWidth();
            const UINT ih = image.GetHeight();
            if (iw > 0 && ih > 0) {
              const double sx = static_cast<double>(c.w - 2) / static_cast<double>(iw);
              const double sy = static_cast<double>(c.h - 2) / static_cast<double>(ih);
              const double sc = std::min(sx, sy);
              const int dw = std::max(1, static_cast<int>(iw * sc));
              const int dh = std::max(1, static_cast<int>(ih * sc));
              const int dx = origin_x + c.x + (c.w - dw) / 2;
              const int dy = origin_y + c.y + (c.h - dh) / 2;
              drawn = g.DrawImage(&image, dx, dy, dw, dh) == Gdiplus::Ok;
            }
          }
        }

        if (!drawn) {
          SetBkMode(hdc, TRANSPARENT);
          SetTextColor(hdc, colorref(c.stroke));
          const std::wstring ws = widen_utf8(c.text.empty() ? std::string("Bild nicht geladen") : c.text);
          TextOutW(hdc, origin_x + c.x + 10, origin_y + c.y + 10, ws.c_str(), static_cast<int>(ws.size()));
          const std::wstring ps = widen_utf8(c.path);
          TextOutW(hdc, origin_x + c.x + 10, origin_y + c.y + 30, ps.c_str(), static_cast<int>(ps.size()));
        }
        break;
      }
    }
  }
}

} // namespace mtx
