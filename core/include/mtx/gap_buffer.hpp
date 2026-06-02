#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace mtx {

class GapBuffer {
public:
  explicit GapBuffer(std::size_t capacity = 4096);
  explicit GapBuffer(const std::string& text);

  std::size_t size() const noexcept;
  std::size_t cursor() const noexcept;
  bool empty() const noexcept;

  void set_text(const std::string& text);
  std::string str() const;
  char at(std::size_t index) const;

  void move_to(std::size_t byte_index);
  void move_left();
  void move_right();

  void insert(char c);
  void insert_text(const std::string& text);
  bool erase_before();
  bool erase_at();
  void erase_range(std::size_t begin, std::size_t end);

private:
  std::vector<char> buf_;
  std::size_t gap_lo_{0};
  std::size_t gap_hi_{0};

  void ensure_gap(std::size_t need);
  void move_gap(std::size_t pos);
};

} // namespace mtx
