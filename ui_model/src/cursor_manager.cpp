#include "mtx/cursor_manager.hpp"

namespace mtx {

CursorVisual CursorManager::visual_for_byte(const std::string& text,
                                            const EditorState& state,
                                            const Viewport& viewport,
                                            const GlyphMetricsTable& metrics,
                                            int text_origin_x,
                                            int text_origin_y) const {
  const LineIndex idx = build_line_index(text);
  const RowCol rc = byte_to_row_col(idx, state.cursor_byte);
  if (rc.row < viewport.first_row || rc.row >= viewport.first_row + viewport.rows) return {};
  if (rc.col < viewport.first_col || rc.col > viewport.first_col + viewport.cols) return {};
  const std::size_t vis_begin = row_col_to_byte(idx, rc.row, viewport.first_col);
  const std::size_t cur = row_col_to_byte(idx, rc.row, rc.col);
  return {true,
          text_origin_x + metrics.measure_bytes(text, vis_begin, cur),
          text_origin_y + (rc.row - viewport.first_row) * metrics.line_height(),
          metrics.line_height()};
}

} // namespace mtx
