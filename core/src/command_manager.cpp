#include "mtx/command_manager.hpp"
#include <algorithm>
#include <cctype>

namespace mtx {

CommandManager::CommandManager(std::size_t max_history) : max_history_(max_history) {}

bool CommandManager::can_undo() const noexcept { return !undo_.empty(); }
bool CommandManager::can_redo() const noexcept { return !redo_.empty(); }
void CommandManager::clear() { undo_.clear(); redo_.clear(); }

void CommandManager::push_undo(EditCommand cmd) {
  undo_.push_back(std::move(cmd));
  if (undo_.size() > max_history_) undo_.erase(undo_.begin());
  redo_.clear();
}

std::string CommandManager::slice(const GapBuffer& buffer, std::size_t begin, std::size_t end) {
  if (begin > end) std::swap(begin, end);
  end = std::min(end, buffer.size());
  std::string out;
  out.reserve(end - begin);
  for (std::size_t i = begin; i < end; ++i) out.push_back(buffer.at(i));
  return out;
}

EditResult CommandManager::insert(GapBuffer& buffer, std::size_t cursor, const std::string& text) {
  if (text.empty()) return {false, cursor};
  cursor = std::min(cursor, buffer.size());
  buffer.move_to(cursor);
  buffer.insert_text(text);
  push_undo(EditCommand{EditKind::Insert, cursor, text, ""});
  return {true, cursor + text.size()};
}

EditResult CommandManager::erase_range(GapBuffer& buffer, std::size_t begin, std::size_t end) {
  if (begin > end) std::swap(begin, end);
  begin = std::min(begin, buffer.size());
  end = std::min(end, buffer.size());
  if (begin == end) return {false, begin};
  const std::string removed = slice(buffer, begin, end);
  buffer.erase_range(begin, end);
  push_undo(EditCommand{EditKind::Erase, begin, "", removed});
  return {true, begin};
}

EditResult CommandManager::backspace(GapBuffer& buffer, std::size_t cursor) {
  if (cursor == 0 || buffer.size() == 0) return {false, cursor};
  return erase_range(buffer, cursor - 1, cursor);
}

EditResult CommandManager::delete_at(GapBuffer& buffer, std::size_t cursor) {
  if (cursor >= buffer.size()) return {false, cursor};
  return erase_range(buffer, cursor, cursor + 1);
}

std::string CommandManager::line_before_cursor(const std::string& text, std::size_t cursor) {
  cursor = std::min(cursor, text.size());
  const std::size_t start = text.rfind('\n', cursor == 0 ? 0 : cursor - 1);
  const std::size_t b = (start == std::string::npos) ? 0 : start + 1;
  return text.substr(b, cursor - b);
}

std::string CommandManager::continuation_prefix(const std::string& line) {
  std::size_t i = 0;
  while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
  const std::string indent = line.substr(0, i);
  if (i + 2 <= line.size() && (line[i] == '-' || line[i] == '*' || line[i] == '+') && line[i + 1] == ' ') {
    std::size_t j = i + 2;
    if (j + 4 <= line.size() && line[j] == '[' && (line[j+1] == ' ' || line[j+1] == 'x' || line[j+1] == 'X') &&
        line[j+2] == ']' && line[j+3] == ' ') {
      // Empty task/list item: terminate the list instead of creating another marker.
      const std::string rest = line.substr(j + 4);
      const bool empty = rest.find_first_not_of(" \t") == std::string::npos;
      return empty ? std::string() : indent + std::string(1, line[i]) + " [ ] ";
    }
    const std::string rest = line.substr(j);
    const bool empty = rest.find_first_not_of(" \t") == std::string::npos;
    return empty ? std::string() : indent + std::string(1, line[i]) + " ";
  }
  // Ordered list: "12. "
  std::size_t n = i;
  while (n < line.size() && std::isdigit(static_cast<unsigned char>(line[n]))) ++n;
  if (n > i && n + 1 < line.size() && line[n] == '.' && line[n+1] == ' ') {
    const std::string rest = line.substr(n + 2);
    if (rest.find_first_not_of(" \t") == std::string::npos) return {};
    int value = 0;
    for (std::size_t k = i; k < n; ++k) value = value * 10 + (line[k] - '0');
    return indent + std::to_string(value + 1) + ". ";
  }
  return {};
}

EditResult CommandManager::smart_newline(GapBuffer& buffer, std::size_t cursor) {
  const std::string text = buffer.str();
  const std::string line = line_before_cursor(text, cursor);
  const std::string ins = "\n" + continuation_prefix(line);
  return insert(buffer, cursor, ins);
}

EditResult CommandManager::undo(GapBuffer& buffer, std::size_t cursor) {
  (void)cursor;
  if (undo_.empty()) return {false, std::min(cursor, buffer.size())};
  EditCommand cmd = undo_.back();
  undo_.pop_back();
  std::size_t next_cursor = cmd.pos;
  if (cmd.kind == EditKind::Insert) {
    buffer.erase_range(cmd.pos, cmd.pos + cmd.inserted.size());
    next_cursor = cmd.pos;
  } else {
    buffer.move_to(cmd.pos);
    buffer.insert_text(cmd.erased);
    next_cursor = cmd.pos + cmd.erased.size();
  }
  redo_.push_back(cmd);
  return {true, next_cursor};
}

EditResult CommandManager::redo(GapBuffer& buffer, std::size_t cursor) {
  (void)cursor;
  if (redo_.empty()) return {false, std::min(cursor, buffer.size())};
  EditCommand cmd = redo_.back();
  redo_.pop_back();
  std::size_t next_cursor = cmd.pos;
  if (cmd.kind == EditKind::Insert) {
    buffer.move_to(cmd.pos);
    buffer.insert_text(cmd.inserted);
    next_cursor = cmd.pos + cmd.inserted.size();
  } else {
    buffer.erase_range(cmd.pos, cmd.pos + cmd.erased.size());
    next_cursor = cmd.pos;
  }
  undo_.push_back(cmd);
  return {true, next_cursor};
}

} // namespace mtx
