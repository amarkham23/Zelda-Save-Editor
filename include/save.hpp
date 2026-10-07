// Copyright (c) 2026 StockiestThree9
#pragma once
#include <cstdint>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace zse {
using Bytes = std::vector<uint8_t>;
struct Error : std::runtime_error {
  using std::runtime_error::runtime_error;
};
uint32_t hashName(const std::string &name);
enum class Game { Botw, Totk };
inline const char *gameName(Game game) {
  return game == Game::Totk ? "Tears of the Kingdom" : "Breath of the Wild";
}
inline const char *saveFilename(Game game) {
  return game == Game::Totk ? "progress.sav" : "game_data.sav";
}
Bytes readFile(const std::string &path, size_t limit = 128 * 1024 * 1024);
void writeFile(const std::string &path, const Bytes &bytes);

// A save is a byte-preserving document. Only existing value words may change.
class Save {
public:
  explicit Save(Bytes data);
  bool bigEndian() const { return be_; }
  uint32_t version() const { return version_; }
  Game game() const { return game_; }
  uint32_t fieldHash(const std::string &name) const;
  const std::map<uint32_t, uint32_t> &types() const { return types_; }
  const Bytes &bytes() const { return data_; }
  const Bytes &original() const { return original_; }
  const std::map<uint32_t, std::vector<size_t>> &fields() const {
    return fields_;
  }
  size_t count(uint32_t hash) const;
  size_t count(const std::string &name) const { return count(fieldHash(name)); }
  uint32_t word(uint32_t hash, size_t index = 0) const;
  int32_t integer(uint32_t hash, size_t index = 0) const;
  float real(uint32_t hash, size_t index = 0) const;
  std::string string(uint32_t hash, size_t width, size_t index = 0) const;
  void word(uint32_t hash, size_t index, uint32_t value);
  void integer(uint32_t hash, size_t index, int32_t value);
  void real(uint32_t hash, size_t index, float value);
  void string(uint32_t hash, size_t width, size_t index,
              const std::string &value);
  size_t logicalCount(uint32_t hash) const;
  bool boolean(uint32_t hash, size_t index = 0) const;
  void boolean(uint32_t hash, size_t index, bool value);
  std::string wideString(uint32_t hash, size_t index = 0) const;
  void wideString(uint32_t hash, size_t index, const std::string &value);
  Bytes binary(uint32_t hash, size_t index = 0) const;
  void binary(uint32_t hash, size_t index, const Bytes &value);
  void edit(const std::string &label, const std::function<void()> &action);
  bool undo();
  bool redo();
  bool dirty() const {
    return editing_ ? data_ != original_ : changedWords_ != 0;
  }
  size_t changedWords() const;
  size_t undoCount() const { return cursor_; }
  std::string undoLabel() const;
  void accepted();

private:
  struct Patch {
    size_t offset;
    uint32_t before, after;
  };
  struct Edit {
    std::string label;
    std::vector<Patch> patches;
  };
  Bytes data_, original_;
  bool be_ = false, editing_ = false;
  uint32_t version_ = 0;
  Game game_ = Game::Botw;
  std::map<uint32_t, uint32_t> types_;
  std::vector<size_t> editableOffsets_;
  std::map<uint32_t, std::vector<size_t>> fields_;
  struct Indirect { uint32_t type; size_t offset, count, width, extent; };
  std::map<uint32_t, Indirect> indirect_;
  std::vector<Edit> edits_;
  size_t cursor_ = 0;
  size_t changedWords_ = 0;
  size_t offset(uint32_t hash, size_t index) const;
  uint32_t read(size_t offset) const;
  void put(size_t offset, uint32_t value);
  void restoreWord(size_t offset, uint32_t value);
};
} // namespace zse
