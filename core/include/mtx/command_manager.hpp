#pragma once
#include "mtx/gap_buffer.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace mtx {

enum class EditKind { Insert, Erase };

struct EditCommand {
  EditKind kind{EditKind::Insert};
  std::size_t pos{0};
  std::string inserted;
  std::string erased;
};

struct EditResult {
  bool changed{false};
  std::size_t cursor{0};
};

class CommandManager {
public:
  explicit CommandManager(std::size_t max_history = 4096);

  EditResult insert(GapBuffer& buffer, std::size_t cursor, const std::string& text);
  EditResult erase_range(GapBuffer& buffer, std::size_t begin, std::size_t end);
  EditResult backspace(GapBuffer& buffer, std::size_t cursor);
  EditResult delete_at(GapBuffer& buffer, std::size_t cursor);
  EditResult smart_newline(GapBuffer& buffer, std::size_t cursor);

  EditResult undo(GapBuffer& buffer, std::size_t cursor);
  EditResult redo(GapBuffer& buffer, std::size_t cursor);

  bool can_undo() const noexcept;
  bool can_redo() const noexcept;
  void clear();

private:
  std::vector<EditCommand> undo_;
  std::vector<EditCommand> redo_;
  std::size_t max_history_{4096};

  void push_undo(EditCommand cmd);
  static std::string slice(const GapBuffer& buffer, std::size_t begin, std::size_t end);
  static std::string line_before_cursor(const std::string& text, std::size_t cursor);
  static std::string continuation_prefix(const std::string& line);
};

} // namespace mtx
