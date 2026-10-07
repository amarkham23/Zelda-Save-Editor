// Copyright (c) 2026 StockiestThree9. MIT.
#include "totk.hpp"
#include <algorithm>
#include <cstring>
namespace zse {
namespace {
// A selective reader avoids constructing the dictionary's 130,000 object flags.
class Dictionary {
  const Bytes &b;
  bool be;
  std::vector<std::string> keys, text;
  uint32_t number(size_t p, size_t n) const {
    if (p > b.size() || n > b.size() - p)
      throw Error("Truncated TOTK dictionary.");
    uint32_t v = 0;
    for (size_t i = 0; i < n; ++i)
      v |= uint32_t(b[p + i]) << (be ? (n - i - 1) * 8 : i * 8);
    return v;
  }
  std::vector<std::string> table(size_t p) const {
    if (number(p, 1) != 0xc2)
      throw Error("Invalid dictionary string table.");
    auto n = number(p + 1, 3);
    if (n > 200000)
      throw Error("Dictionary string table is too large.");
    std::vector<std::string> out;
    for (size_t i = 0; i < n; ++i) {
      size_t at = p + number(p + 4 + i * 4, 4), end = at;
      while (end < b.size() && b[end] && end - at < 4096)
        ++end;
      if (end >= b.size() || end - at >= 4096)
        throw Error("Invalid dictionary string.");
      out.emplace_back(b.begin() + at, b.begin() + end);
    }
    return out;
  }

public:
  explicit Dictionary(const Bytes &bytes) : b(bytes), be(bytes.size() > 1 && bytes[0] == 'B') {
    if (b.size() < 16 || (!be && (b[0] != 'Y' || b[1] != 'B')) || (be && b[1] != 'Y') ||
        number(2, 2) < 2 || number(2, 2) > 7)
      throw Error("Invalid TOTK dictionary header.");
    keys = table(number(4, 4));
    text = table(number(8, 4));
  }
  struct Value {
    uint8_t type;
    uint32_t raw;
  };
  Value root() const {
    auto p = number(12, 4);
    return {uint8_t(number(p, 1)), p};
  }
  std::map<std::string, Value> object(Value v) const {
    if (v.type != 0xc1 || number(v.raw, 1) != 0xc1)
      throw Error("Invalid dictionary object.");
    size_t n = number(v.raw + 1, 3);
    if (n > 200000)
      throw Error("Dictionary object is too large.");
    std::map<std::string, Value> out;
    for (size_t i = 0; i < n; ++i) {
      size_t p = v.raw + 4 + i * 8, k = number(p, 3);
      if (k >= keys.size())
        throw Error("Invalid dictionary key.");
      out.emplace(keys[k], Value{uint8_t(number(p + 3, 1)), number(p + 4, 4)});
    }
    return out;
  }
  std::vector<Value> array(Value v) const {
    if (v.type != 0xc0 || number(v.raw, 1) != 0xc0)
      throw Error("Invalid dictionary array.");
    size_t n = number(v.raw + 1, 3);
    if (n > 200000)
      throw Error("Dictionary array is too large.");
    size_t p = (v.raw + 4 + n + 3) & ~size_t(3);
    std::vector<Value> out;
    out.reserve(n);
    for (size_t i = 0; i < n; ++i)
      out.push_back({uint8_t(number(v.raw + 4 + i, 1)), number(p + i * 4, 4)});
    return out;
  }
  uint32_t scalar(Value v) const {
    if (v.type == 0xd4 || v.type == 0xd5) {
      uint32_t hi = number(v.raw + (be ? 0 : 4), 4), lo = number(v.raw + (be ? 4 : 0), 4);
      if (hi)
        throw Error("Oversized enum hash.");
      return lo;
    }
    if (v.type != 0xd1 && v.type != 0xd3)
      throw Error("Invalid enum hash type.");
    return v.raw;
  }
  std::string string(Value v) const {
    if (v.type != 0xa0 || v.raw >= text.size())
      throw Error("Invalid dictionary enum name.");
    return text[v.raw];
  }
};
std::string text(const ByNode &n, const char *key) {
  auto p = n.get(key);
  return p && p->type == 0xa0 ? p->text : std::string{};
}
} // namespace
void readTotkEnums(Metadata &metadata, const Bytes &bytes) {
  Dictionary reader(bytes);
  auto root = reader.object(reader.root());
  auto data = root.find("Data");
  if (data == root.end())
    throw Error("TOTK dictionary has no data.");
  auto sections = reader.object(data->second);
  decltype(metadata.enums) next;
  for (auto name : {"Enum", "EnumArray"}) {
    auto it = sections.find(name);
    if (it == sections.end())
      continue;
    for (auto v : reader.array(it->second)) {
      auto f = reader.object(v);
      auto h = f.find("Hash"), raw = f.find("RawValues"), values = f.find("Values");
      if (h == f.end() || raw == f.end() || values == f.end())
        continue;
      auto labels = reader.array(raw->second), hashes = reader.array(values->second);
      if (labels.size() != hashes.size() || labels.size() > 256)
        throw Error("Invalid dictionary enum choices.");
      std::vector<std::pair<uint32_t, std::string>> choices;
      for (size_t i = 0; i < labels.size(); ++i) {
        auto label = reader.string(labels[i]);
        if (totk::hash(label) != reader.scalar(hashes[i]))
          throw Error("Dictionary enum mismatch: " + label + " expected " +
                      std::to_string(reader.scalar(hashes[i])) + " got " +
                      std::to_string(totk::hash(label)));
        choices.emplace_back(reader.scalar(hashes[i]), readable(label));
      }
      next.emplace(reader.scalar(h->second), std::move(choices));
    }
  }
  metadata.enums = std::move(next);
}
void readTotkChallenges(Metadata &metadata, const Bytes &bytes) {
  auto root = readByml(bytes);
  if (root.type != 0xc0)
    throw Error("Invalid TOTK quest table.");
  decltype(metadata.quests) next;
  for (auto &row : root.array) {
    auto path = text(row, "__RowId");
    auto start = path.find_last_of('/'), end = path.find(".game__challenge__Challenge.");
    if (start == std::string::npos || end == std::string::npos || end <= start)
      continue;
    auto id = path.substr(start + 1, end - start - 1);
    auto steps = row.get("Steps");
    if (!steps)
      continue;
    std::vector<std::string> names;
    for (auto &step : steps->array) {
      auto name = text(step, "Name");
      if (!name.empty())
        names.push_back(id + "_" + name);
      auto force = text(step, "ForceProgressGameData");
      if (!force.empty())
        names.push_back(force);
    }
    if (!names.empty())
      next.emplace(id, std::move(names));
  }
  metadata.quests = std::move(next);
}
void readTotkAttachments(Metadata &metadata, const Bytes &bytes) {
  auto root = readByml(bytes);
  if (root.type != 0xc0)
    throw Error("Invalid TOTK attachment table.");
  decltype(metadata.attachments) next;
  for (auto &row : root.array) {
    auto id = text(row, "__RowId");
    if (id.empty() || id.size() >= 64)
      continue;
    auto v = row.get("AttachmentAdditionalDamage");
    next.emplace(id, v && v->number < 100000 ? int(v->number) : 0);
  }
  metadata.attachments = std::move(next);
}
} // namespace zse
