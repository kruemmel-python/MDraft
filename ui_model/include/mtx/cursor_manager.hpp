#pragma once
#include "mtx/cursor_controller.hpp"
#include "mtx/editor_state.hpp"
#include "mtx/glyph_metrics_table.hpp"
#include "mtx/viewport_manager.hpp"
#include <string>

namespace mtx {

struct CursorVisual {
  bool visible{false};
  int x{0};
  int y{0};
  int h{0};
};

class CursorManager {
public:
  CursorVisual visual_for_byte(const std::string& text,
                               const EditorState& state,
                               const Viewport& viewport,
                               const GlyphMetricsTable& metrics,
                               int text_origin_x,
                               int text_origin_y) const;
};

} // namespace mtx
