// Copyright (c) 2026 StockiestThree9
#pragma once
#include "resources.hpp"
#include <array>
namespace zse {
struct EditField {
  Field field;
  size_t index = 0;
  std::string label, description;
  std::string unit;
  double scale = 1, minimum = -2147483648.0, maximum = 2147483647.0;
  bool whole = false;
  std::vector<std::pair<uint32_t, std::string>> choices;
};
std::string displayValue(const Save &save, const EditField &field);
void setValue(Save &save, const EditField &field, const std::string &text);
std::string readable(const std::string &name);
enum class ItemGroup {
  Weapons,
  Bows,
  Arrows,
  Shields,
  Armor,
  Materials,
  Food,
  KeyItems,
  Unknown
};
ItemGroup itemGroup(const std::string &id);
std::string groupName(ItemGroup group);
struct Item {
  std::string id;
  size_t index = 0, ordinal = 0;
  ItemGroup group = ItemGroup::Unknown;
};
class Botw {
public:
  explicit Botw(const Metadata &m) : metadata(m) {}
  const Metadata &metadata;
  std::vector<EditField> stats(const Save &s) const;
  std::vector<Item> items(const Save &s) const;
  std::vector<EditField> itemFields(const Save &s, const Item &item) const;
  std::vector<EditField> horseFields(const Save &s, size_t horse) const;
  std::vector<EditField>
  matching(const Save &s,
           const std::function<bool(const Field &)> &predicate) const;
  std::vector<EditField> elements(const Save &s, const Field &field) const;
  void replaceItem(Save &s, const Item &item, const std::string &id) const;
  void addItem(Save &s, const std::string &id) const;
  void removeItem(Save &s, const Item &item) const;
  void equipItem(Save &s, const Item &item) const;
  void resetAmiibo(Save &s) const;

private:
  void validateItem(const Save &save, const Item &item) const;
  EditField field(const std::string &name, const std::string &label,
                  size_t index = 0) const;
  struct Entry {
    std::string id;
    uint32_t value = 0, equipped = 0, flags = 0, modifier = 0;
    std::array<uint32_t, 6> food{};
    std::array<std::string, 5> ingredients;
  };
  std::vector<Entry> inventory(const Save &s) const;
  void inventory(Save &s, const std::vector<Entry> &entries) const;
};
} // namespace zse
