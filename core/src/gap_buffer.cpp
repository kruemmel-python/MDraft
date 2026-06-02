#include "mtx/gap_buffer.hpp"
#include <algorithm>
#include <stdexcept>

namespace mtx {

GapBuffer::GapBuffer(std::size_t capacity) : buf_(capacity), gap_lo_(0), gap_hi_(capacity) {}

GapBuffer::GapBuffer(const std::string& text) {
  buf_.assign(text.size() + 4096, 0);
  std::copy(text.begin(), text.end(), buf_.begin());
  gap_lo_ = text.size();
  gap_hi_ = buf_.size();
}

std::size_t GapBuffer::size() const noexcept { return buf_.size() - (gap_hi_ - gap_lo_); }
std::size_t GapBuffer::cursor() const noexcept { return gap_lo_; }
bool GapBuffer::empty() const noexcept { return size() == 0; }

void GapBuffer::set_text(const std::string& text) {
  buf_.assign(text.size() + 4096, 0);
  std::copy(text.begin(), text.end(), buf_.begin());
  gap_lo_ = text.size();
  gap_hi_ = buf_.size();
}

std::string GapBuffer::str() const {
  std::string out;
  out.reserve(size());
  out.append(buf_.data(), gap_lo_);
  out.append(buf_.data() + gap_hi_, buf_.size() - gap_hi_);
  return out;
}

char GapBuffer::at(std::size_t index) const {
  if (index >= size()) throw std::out_of_range("GapBuffer::at");
  return index < gap_lo_ ? buf_[index] : buf_[index + (gap_hi_ - gap_lo_)];
}

void GapBuffer::ensure_gap(std::size_t need) {
  const std::size_t gap = gap_hi_ - gap_lo_;
  if (gap >= need) return;

  const std::size_t old_size = size();
  const std::size_t new_gap = std::max<std::size_t>(need + 4096, old_size / 2 + need);
  std::vector<char> next(old_size + new_gap);

  std::copy(buf_.begin(), buf_.begin() + static_cast<long>(gap_lo_), next.begin());
  const std::size_t next_gap_hi = gap_lo_ + new_gap;
  std::copy(buf_.begin() + static_cast<long>(gap_hi_), buf_.end(), next.begin() + static_cast<long>(next_gap_hi));

  buf_.swap(next);
  gap_hi_ = next_gap_hi;
}

void GapBuffer::move_gap(std::size_t pos) {
  if (pos > size()) pos = size();
  if (pos < gap_lo_) {
    const std::size_t n = gap_lo_ - pos;
    std::move_backward(buf_.begin() + static_cast<long>(pos),
                       buf_.begin() + static_cast<long>(gap_lo_),
                       buf_.begin() + static_cast<long>(gap_hi_));
    gap_lo_ -= n;
    gap_hi_ -= n;
  } else if (pos > gap_lo_) {
    const std::size_t n = pos - gap_lo_;
    std::move(buf_.begin() + static_cast<long>(gap_hi_),
              buf_.begin() + static_cast<long>(gap_hi_ + n),
              buf_.begin() + static_cast<long>(gap_lo_));
    gap_lo_ += n;
    gap_hi_ += n;
  }
}

void GapBuffer::move_to(std::size_t byte_index) { move_gap(byte_index); }
void GapBuffer::move_left() { if (gap_lo_ > 0) move_gap(gap_lo_ - 1); }
void GapBuffer::move_right() { if (gap_lo_ < size()) move_gap(gap_lo_ + 1); }

void GapBuffer::insert(char c) {
  ensure_gap(1);
  buf_[gap_lo_++] = c;
}

void GapBuffer::insert_text(const std::string& text) {
  ensure_gap(text.size());
  for (char c : text) buf_[gap_lo_++] = c;
}

bool GapBuffer::erase_before() {
  if (gap_lo_ == 0) return false;
  --gap_lo_;
  return true;
}

bool GapBuffer::erase_at() {
  if (gap_hi_ == buf_.size()) return false;
  ++gap_hi_;
  return true;
}

void GapBuffer::erase_range(std::size_t begin, std::size_t end) {
  if (begin > end) std::swap(begin, end);
  if (end > size()) end = size();
  move_gap(begin);
  gap_hi_ += (end - begin);
}

} // namespace mtx
