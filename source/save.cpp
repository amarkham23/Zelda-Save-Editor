// Copyright (c) 2026 StockiestThree9
#include "save.hpp"
#include "totk_format.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>

namespace zse {
uint32_t hashName(const std::string &name) {
  uint32_t crc = 0xffffffff;
  for (unsigned char ch : name) {
    crc ^= ch;
    for (int i = 0; i < 8; ++i)
      crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1)));
  }
  return ~crc;
}
Bytes readFile(const std::string &path, size_t limit) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f)
    throw Error("Could not open " + path);
  auto n = f.tellg();
  if (n < 0 || static_cast<uint64_t>(n) > limit)
    throw Error("File size is invalid: " + path);
  Bytes b(static_cast<size_t>(n));
  f.seekg(0);
  if (!b.empty() && !f.read(reinterpret_cast<char *>(b.data()), b.size()))
    throw Error("Could not read " + path);
  return b;
}
void writeFile(const std::string &path, const Bytes &bytes) {
  std::ofstream f(path, std::ios::binary | std::ios::trunc);
  if (!f ||
      !f.write(reinterpret_cast<const char *>(bytes.data()), bytes.size()))
    throw Error("Could not write " + path);
  f.flush();
  if (!f)
    throw Error("Could not flush " + path);
  f.close();
  if (f.fail())
    throw Error("Could not close " + path);
}
Save::Save(Bytes data) : data_(std::move(data)), original_(data_) {
  if (data_.size() >= 12 && read(0) == 0x01020304) {
    game_ = Game::Totk;
    totk::Save parsed(data_);
    version_ = parsed.version();
    if (version_ != 0x46c3c8 && version_ != 0x47e0f4 && version_ != 0x49e946)
      throw Error("This TOTK save revision is not supported yet.");
    for (auto &pair : parsed.fields()) {
      auto &f = pair.second;
      uint32_t type = uint32_t(f.type);
      types_[pair.first] = type;
      // Map only directly editable 32-bit cells. Packed booleans, binary
      // payloads, wide strings and 64-bit keys remain preserved and hidden.
      if (type == 1 || type >= 18)
        continue;
      size_t cells = type == 0 ? 1 : f.width / 4 * f.count;
      if (type == 0)
        cells = 1;
      auto &offsets = fields_[pair.first];
      offsets.reserve(cells);
      for (size_t i = 0; i < cells; ++i) {
        offsets.push_back(f.offset + i * 4);
        editableOffsets_.push_back(f.offset + i * 4);
      }
    }
    // UInt/UIntArray are also direct cells (types 20/21).
    for (auto &pair : parsed.fields())
      if (pair.second.type == totk::Type::UInt ||
          pair.second.type == totk::Type::UIntArray) {
        auto &f = pair.second;
        for (size_t i = 0; i < f.count; ++i) {
          fields_[pair.first].push_back(f.offset + i * 4);
          editableOffsets_.push_back(f.offset + i * 4);
        }
      }
    std::sort(editableOffsets_.begin(), editableOffsets_.end());
    editableOffsets_.erase(
        std::unique(editableOffsets_.begin(), editableOffsets_.end()),
        editableOffsets_.end());
    return;
  }
  if (data_.size() < 24 || data_.size() > 16 * 1024 * 1024 ||
      (data_.size() - 16) % 8)
    throw Error("This is not a complete BOTW save.");
  if (data_[8] == 0 && data_[11] == 1)
    be_ = true;
  else if (!(data_[8] == 1 && data_[11] == 0))
    throw Error("Invalid save byte order.");
  version_ = read(0);
  const uint32_t known[] = {0x24e2, 0x24ee, 0x2588, 0x29c0, 0x2a46, 0x3ef8,
                            0x3ef9, 0x471a, 0x471b, 0x471e, 0x4730};
  if (std::find(std::begin(known), std::end(known), version_) ==
      std::end(known))
    throw Error("This save version is not supported yet.");
  if (read(4) != 0xffffffff || read(8) != 1 ||
      read(data_.size() - 4) != 0xffffffff)
    throw Error("Invalid save header or end marker.");
  for (size_t p = 12; p + 8 <= data_.size() - 4; p += 8) {
    uint32_t h = read(p);
    if (h == 0xffffffff)
      throw Error("Unexpected end marker inside save.");
    fields_[h].push_back(p + 4);
    editableOffsets_.push_back(p + 4);
  }
}
uint32_t Save::fieldHash(const std::string &name) const {
  return game_ == Game::Totk ? totk::hash(name) : hashName(name);
}
uint32_t Save::read(size_t p) const {
  if (p > data_.size() || data_.size() - p < 4)
    throw Error("Save offset is out of bounds.");
  uint32_t v = 0;
  for (int i = 0; i < 4; ++i)
    v |= uint32_t(data_[p + i]) << (be_ ? (3 - i) * 8 : i * 8);
  return v;
}
void Save::put(size_t p, uint32_t v) {
  for (int i = 0; i < 4; ++i)
    data_[p + i] = uint8_t(v >> (be_ ? (3 - i) * 8 : i * 8));
}
size_t Save::count(uint32_t h) const {
  auto it = fields_.find(h);
  return it == fields_.end() ? 0 : it->second.size();
}
size_t Save::offset(uint32_t h, size_t i) const {
  auto it = fields_.find(h);
  if (it == fields_.end() || i >= it->second.size())
    throw Error("That field or array element is absent from this save.");
  return it->second[i];
}
uint32_t Save::word(uint32_t h, size_t i) const { return read(offset(h, i)); }
int32_t Save::integer(uint32_t h, size_t i) const {
  uint32_t u = word(h, i);
  int32_t v;
  std::memcpy(&v, &u, 4);
  return v;
}
float Save::real(uint32_t h, size_t i) const {
  uint32_t u = word(h, i);
  float v;
  std::memcpy(&v, &u, 4);
  return v;
}
std::string Save::string(uint32_t h, size_t width, size_t index) const {
  if (!width || width % 4 || index >= count(h) / (width / 4))
    throw Error("Invalid string index.");
  std::string result;
  for (size_t w = 0; w < width / 4; ++w) {
    size_t p = offset(h, index * (width / 4) + w);
    for (size_t b = 0; b < 4; ++b) {
      char c = char(data_[p + b]);
      if (!c)
        return result;
      result += c;
    }
  }
  return result;
}
void Save::word(uint32_t h, size_t i, uint32_t v) {
  if (!editing_)
    throw Error("An edit transaction is required.");
  put(offset(h, i), v);
}
void Save::integer(uint32_t h, size_t i, int32_t v) {
  uint32_t u;
  std::memcpy(&u, &v, 4);
  word(h, i, u);
}
void Save::real(uint32_t h, size_t i, float v) {
  if (!std::isfinite(v))
    throw Error("Enter a finite number.");
  uint32_t u;
  std::memcpy(&u, &v, 4);
  word(h, i, u);
}
void Save::string(uint32_t h, size_t width, size_t index,
                  const std::string &value) {
  if (!editing_)
    throw Error("An edit transaction is required.");
  if (!width || width % 4 || index >= count(h) / (width / 4) ||
      value.size() >= width || value.find('\0') != std::string::npos)
    throw Error("The text is too long for this field.");
  for (size_t w = 0; w < width / 4; ++w) {
    size_t p = offset(h, index * (width / 4) + w);
    for (size_t b = 0; b < 4; ++b) {
      size_t j = w * 4 + b;
      data_[p + b] = j < value.size() ? uint8_t(value[j]) : 0;
    }
  }
}
void Save::edit(const std::string &label, const std::function<void()> &action) {
  if (editing_)
    throw Error("Nested edits are not supported.");
  Bytes before = data_;
  Edit e{label, {}};
  size_t changes = 0;
  editing_ = true;
  try {
    action();
    for (size_t p : editableOffsets_) {
      changes += std::memcmp(&original_[p], &data_[p], 4) != 0;
      if (std::memcmp(&before[p], &data_[p], 4) == 0)
        continue;
      uint32_t old = 0;
      for (int i = 0; i < 4; ++i)
        old |= uint32_t(before[p + i]) << (be_ ? (3 - i) * 8 : i * 8);
      e.patches.push_back({p, old, read(p)});
    }
    if (e.patches.empty()) {
      editing_ = false;
      return;
    }
    // Allocate before discarding redo history. If journal construction fails,
    // restore the document just as for a failed edit action.
    edits_.reserve(cursor_ + 1);
    edits_.resize(cursor_);
    edits_.push_back(std::move(e));
    cursor_ = edits_.size();
  } catch (...) {
    data_.swap(before);
    editing_ = false;
    throw;
  }
  editing_ = false;
  changedWords_ = changes;
  // A bounded history prevents a long editing session from exhausting applet
  // memory.
  if (edits_.size() > 100) {
    edits_.erase(edits_.begin());
    --cursor_;
  }
}
void Save::restoreWord(size_t p, uint32_t value) {
  bool before = std::memcmp(&original_[p], &data_[p], 4) != 0;
  put(p, value);
  bool after = std::memcmp(&original_[p], &data_[p], 4) != 0;
  if (before != after) {
    if (after)
      ++changedWords_;
    else
      --changedWords_;
  }
}
bool Save::undo() {
  if (editing_)
    throw Error("Finish the edit before changing history.");
  if (!cursor_)
    return false;
  for (auto &p : edits_[--cursor_].patches)
    restoreWord(p.offset, p.before);
  return true;
}
bool Save::redo() {
  if (editing_)
    throw Error("Finish the edit before changing history.");
  if (cursor_ == edits_.size())
    return false;
  for (auto &p : edits_[cursor_++].patches)
    restoreWord(p.offset, p.after);
  return true;
}
std::string Save::undoLabel() const {
  return cursor_ ? edits_[cursor_ - 1].label : "";
}
size_t Save::changedWords() const {
  if (!editing_)
    return changedWords_;
  size_t n = 0;
  for (size_t p : editableOffsets_)
    n += std::memcmp(&original_[p], &data_[p], 4) != 0;
  return n;
}
void Save::accepted() {
  if (editing_)
    throw Error("Finish the edit before accepting the save.");
  original_ = data_;
  edits_.clear();
  cursor_ = changedWords_ = 0;
}
} // namespace zse
