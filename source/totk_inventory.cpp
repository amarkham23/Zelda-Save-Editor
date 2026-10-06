// Copyright (c) 2026 StockiestThree9. MIT.
#include "totk.hpp"
#include <algorithm>
#include <set>

namespace zse {
namespace {
const char *sections[] = {"Weapon",   "Bow",  "Arrow",   "Shield",      "Armor",
                          "Material", "Food", "KeyItem", "SpecialParts"};
std::string pouch(ItemGroup group) {
  if (unsigned(group) >= 9)
    throw Error("Unknown inventory section.");
  return std::string("Pouch.") + sections[unsigned(group)] + ".";
}
struct Column {
  std::string name;
  size_t words;
};
std::vector<Column> columns(const Save &s, ItemGroup group, size_t capacity) {
  auto p = pouch(group) + "Content.";
  std::vector<Column> result;
  auto add = [&](const char *name, size_t width, uint32_t type) {
    auto key = p + name;
    auto h = totk::hash(key);
    if (!s.count(h) || s.types().at(h) != type ||
        s.count(h) != capacity * 2 * width)
      throw Error(
          "This inventory layout is not supported for item management.");
    result.push_back({key, width});
  };
  add("Name", 16, 17);
  if (group == ItemGroup::Weapons || group == ItemGroup::Bows ||
      group == ItemGroup::Shields) {
    add("Life", 1, 3);
    add("Effect.Type", 1, 7);
    add("Effect.Value", 1, 3);
    if (group != ItemGroup::Bows) {
      add("Combined.Name", 16, 17);
      add("Combined.Life", 1, 3);
      add("ExtraLife", 1, 3);
      if (group == ItemGroup::Weapons)
        add("RecordExtraLife", 1, 3);
    }
  } else if (group == ItemGroup::Armor)
    add("ColorVariation", 1, 7);
  else {
    add("StockNum", 1, 3);
    if (group == ItemGroup::Materials) {
      add("GetOrder", 1, 3);
      add("UseOrder", 1, 3);
    }
    if (group == ItemGroup::Unknown)
      add("UseOrder", 1, 3);
    if (group == ItemGroup::Food) {
      add("LifeRecover", 1, 3);
      add("Effect.Type", 1, 7);
      add("Effect.Level", 1, 3);
      add("Effect.Time", 1, 3);
      add("Price", 1, 3);
      add("MaterialName", 80, 17);
    }
  }
  return result;
}
void copySlot(Save &s, const std::vector<Column> &cols, size_t from,
              size_t to) {
  for (auto &c : cols)
    for (size_t i = 0; i < c.words; ++i)
      s.word(totk::hash(c.name), to * c.words + i,
             s.word(totk::hash(c.name), from * c.words + i));
}
void resetSlot(Save &s, const std::vector<Column> &cols, size_t slot) {
  for (auto &c : cols) {
    uint32_t value = 0;
    if (c.name.find(".Effect.Type") != std::string::npos ||
        c.name.find(".ColorVariation") != std::string::npos)
      value = totk::hash("None");
    if (c.name.find(".Combined.Life") != std::string::npos ||
        c.name.find(".GetOrder") != std::string::npos ||
        c.name.find(".UseOrder") != std::string::npos)
      value = uint32_t(-1);
    for (size_t i = 0; i < c.words; ++i)
      s.word(totk::hash(c.name), slot * c.words + i, value);
  }
}
std::vector<std::string> references(ItemGroup group) {
  auto p = pouch(group);
  if (group == ItemGroup::Armor)
    return {p + "EquipIndexes.Head", p + "EquipIndexes.Upper",
            p + "EquipIndexes.Bottom"};
  if (group == ItemGroup::Food || group == ItemGroup::KeyItems)
    return {};
  return {p + "EquipIndex"};
}
void validateItem(const Save &s, const Item &item, size_t capacity) {
  if (item.ordinal >= capacity ||
      s.string(totk::hash(pouch(item.group) + "Content.Name"), 64,
               item.ordinal) != item.id)
    throw Error("That item changed. Return to Inventory and select it again.");
}
bool starts(const std::string &value, const std::string &prefix) {
  return value.rfind(prefix, 0) == 0;
}
bool belongs(const std::string &id, ItemGroup group) {
  switch (group) {
  case ItemGroup::Weapons:
    return starts(id, "Weapon_Sword_") || starts(id, "Weapon_Lsword_") ||
           starts(id, "Weapon_Spear_");
  case ItemGroup::Bows:
    return starts(id, "Weapon_Bow_");
  case ItemGroup::Arrows:
    return id == "NormalArrow";
  case ItemGroup::Shields:
    return starts(id, "Weapon_Shield_");
  case ItemGroup::Armor:
    return starts(id, "Armor_");
  case ItemGroup::Materials:
    return (starts(id, "Item_") && !starts(id, "Item_Cook_") &&
            !starts(id, "Item_Roast_") && !starts(id, "Item_Chilled_")) ||
           starts(id, "Animal_Insect_") || starts(id, "LightBall_") ||
           id == "BombFruit" || id == "FireFruit" || id == "WaterFruit" ||
           id == "ElectricalFruit" || id == "IceFruit" || id == "Obj_FirewoodBundle" || id == "BeeHome";
  case ItemGroup::Food:
    return starts(id, "Item_Cook_") || starts(id, "Item_Roast_") ||
           starts(id, "Item_Chilled_");
  case ItemGroup::Unknown:
    return starts(id, "SpObj_") && id.find("_Capsule_") != std::string::npos;
  default:
    return false;
  }
}
bool specialWeapon(const std::string &id) {
  return id == "Weapon_Sword_070" || id == "Weapon_Sword_071" ||
         id == "Weapon_Sword_001_Decayed";
}
void checkManage(const Totk &model, const Item &item) {
  if (!model.canManage(item))
    throw Error("This special item's identity is protected. Its existing "
                "values can still be edited.");
}
} // namespace
size_t Totk::capacity(const Save &s, ItemGroup group) const {
  if (s.game() != Game::Totk)
    throw Error("Choose a TOTK save.");
  auto h = totk::hash(pouch(group) + "Content.Name");
  auto count = s.count(h);
  if (!count || count % 32 || s.types().at(h) != 17)
    throw Error("Unknown TOTK inventory layout.");
  return count / 32;
}
size_t Totk::availableSlots(const Save &s, ItemGroup group) const {
  size_t limit = capacity(s, group);
  auto key = pouch(group) + "ValidNum";
  if (s.count(key)) {
    if (s.count(key) != 2)
      throw Error("Unknown pouch capacity layout.");
    limit = std::min(limit, size_t(std::max(0, s.integer(totk::hash(key)))));
  }
  size_t occupied = 0;
  auto name = totk::hash(pouch(group) + "Content.Name");
  for (size_t i = 0; i < limit; ++i)
    occupied += !s.string(name, 64, i).empty();
  return limit - occupied;
}
bool Totk::canManage(ItemGroup group) const {
  return group != ItemGroup::KeyItems;
}
bool Totk::canManage(const Item &item) const {
  return canManage(item.group) && !specialWeapon(item.id);
}
bool Totk::canDuplicate(ItemGroup group) const {
  return group == ItemGroup::Weapons || group == ItemGroup::Bows ||
         group == ItemGroup::Shields || group == ItemGroup::Armor ||
         group == ItemGroup::Food;
}
std::vector<std::string> Totk::catalog(const Save &s, ItemGroup group) const {
  std::set<std::string> ids;
  if (!canManage(group))
    return {};
  for (auto &pair : metadata.names) {
    auto h = totk::hash("IsGet." + pair.first);
    auto it = s.types().find(h);
    if (belongs(pair.first, group) && !specialWeapon(pair.first) &&
        s.count(h) == 1 && it != s.types().end() && it->second == 0)
      ids.insert(pair.first);
  }
  // Existing items remain usable when the installed message archive is
  // unavailable.
  for (auto &pair : originalItems)
    if (pair.second == group && !specialWeapon(pair.first))
      ids.insert(pair.first);
  for (auto &item : items(s))
    if (item.group == group && !specialWeapon(item.id))
      ids.insert(item.id);
  return {ids.begin(), ids.end()};
}
Item Totk::addItem(Save &s, ItemGroup group, const std::string &id) const {
  auto choices = catalog(s, group);
  if (std::find(choices.begin(), choices.end(), id) == choices.end())
    throw Error("Choose an item from this game's catalogue.");
  auto cap = capacity(s, group);
  auto cols = columns(s, group, cap);
  auto key = pouch(group) + "Content.";
  if (!availableSlots(s, group))
    throw Error("This pouch is full. Remove an item or increase its capacity "
                "in Player.");
  size_t limit = cap;
  if (s.count(pouch(group) + "ValidNum"))
    limit = std::min(
        cap,
        size_t(std::max(0, s.integer(totk::hash(pouch(group) + "ValidNum")))));
  size_t target = 0;
  while (target < limit &&
         !s.string(totk::hash(key + "Name"), 64, target).empty())
    ++target;
  if (group == ItemGroup::Materials || group == ItemGroup::Arrows ||
      group == ItemGroup::Unknown) {
    for (size_t i = 0; i < cap; ++i)
      if (s.string(totk::hash(key + "Name"), 64, i) == id)
        throw Error("You already have this item. Edit its quantity instead.");
  }
  s.edit("Add " + metadata.itemName(id), [&] {
    resetSlot(s, cols, target);
    s.string(totk::hash(key + "Name"), 64, target, id);
    if (s.count(key + "StockNum"))
      s.integer(totk::hash(key + "StockNum"), target, 1);
    if (s.count(key + "Life"))
      s.integer(totk::hash(key + "Life"), target, initialDurability(id));
  });
  return {id, size_t(group) * 4096 + target, target, group};
}
void Totk::replaceItem(Save &s, const Item &item, const std::string &id) const {
  checkManage(*this, item);
  validateItem(s, item, capacity(s, item.group));
  auto choices = catalog(s, item.group);
  columns(s, item.group, capacity(s, item.group));
  if (std::find(choices.begin(), choices.end(), id) == choices.end())
    throw Error("Choose an item from the same section.");
  if (item.id == id)
    return;
  // Cross-slot armor replacement requires a different equipped reference.
  auto part = [](const std::string &n) { return n.substr(n.rfind('_') + 1); };
  if (item.group == ItemGroup::Armor && part(item.id) != part(id))
    throw Error("Choose armor for the same body part.");
  if (item.group == ItemGroup::Materials || item.group == ItemGroup::Arrows ||
      item.group == ItemGroup::Unknown)
    for (auto &other : items(s))
      if (other.group == item.group && other.id == id)
        throw Error("That item already has a stack.");
  s.edit("Replace " + metadata.itemName(item.id), [&] {
    s.string(totk::hash(pouch(item.group) + "Content.Name"), 64, item.ordinal,
             id);
  });
}
void Totk::duplicateItem(Save &s, const Item &item) const {
  checkManage(*this, item);
  if (!canDuplicate(item.group))
    throw Error("Change this item's quantity instead.");
  auto cap = capacity(s, item.group);
  validateItem(s, item, cap);
  auto cols = columns(s, item.group, cap);
  if (!availableSlots(s, item.group))
    throw Error("This pouch is full. Remove an item or increase its capacity "
                "in Player.");
  auto name = totk::hash(pouch(item.group) + "Content.Name");
  size_t target = 0;
  while (target < cap && !s.string(name, 64, target).empty())
    ++target;
  s.edit("Duplicate " + metadata.itemName(item.id),
         [&] { copySlot(s, cols, item.ordinal, target); });
}
void Totk::removeItem(Save &s, const Item &item) const {
  checkManage(*this, item);
  auto cap = capacity(s, item.group);
  validateItem(s, item, cap);
  auto cols = columns(s, item.group, cap);
  for (auto &ref : references(item.group))
    if (s.count(ref) != 2)
      throw Error("Unknown equipped-item layout.");
  s.edit("Remove " + metadata.itemName(item.id), [&] {
    for (size_t i = item.ordinal; i + 1 < cap; ++i)
      copySlot(s, cols, i + 1, i);
    resetSlot(s, cols, cap - 1);
    for (auto &ref : references(item.group)) {
      auto h = totk::hash(ref);
      auto index = s.integer(h);
      if (index == int(item.ordinal))
        s.integer(h, 0, -1);
      else if (index > int(item.ordinal) && index < int(cap))
        s.integer(h, 0, index - 1);
    }
  });
}
bool Totk::equipped(const Save &s, const Item &item) const {
  for (auto &ref : references(item.group))
    if (s.count(ref) == 2 && s.integer(totk::hash(ref)) == int(item.ordinal))
      return true;
  return false;
}
void Totk::equipItem(Save &s, const Item &item) const {
  validateItem(s, item, capacity(s, item.group));
  if (item.group != ItemGroup::Weapons && item.group != ItemGroup::Bows &&
      item.group != ItemGroup::Shields && item.group != ItemGroup::Armor)
    throw Error("This item cannot be equipped here.");
  auto refs = references(item.group);
  std::string key = refs.front();
  if (item.group == ItemGroup::Armor) {
    auto part = item.id.substr(item.id.rfind('_') + 1);
    if (part == "Head")
      key = refs[0];
    else if (part == "Upper")
      key = refs[1];
    else if (part == "Lower")
      key = refs[2];
    else
      throw Error("Unknown armor body part.");
  }
  if (s.count(key) != 2)
    throw Error("Unknown equipped-item layout.");
  bool active = equipped(s, item);
  s.edit(active ? "Unequip item" : "Equip item", [&] {
    s.integer(totk::hash(key), 0, active ? -1 : int32_t(item.ordinal));
  });
}
void Totk::clearFusion(Save &s, const Item &item) const {
  validateItem(s, item, capacity(s, item.group));
  if (item.group != ItemGroup::Weapons && item.group != ItemGroup::Shields)
    throw Error("This item does not support fusion.");
  auto p = pouch(item.group) + "Content.";
  columns(s, item.group, capacity(s, item.group));
  s.edit("Remove fused material", [&] {
    s.string(totk::hash(p + "Combined.Name"), 64, item.ordinal, "");
    s.integer(totk::hash(p + "Combined.Life"), item.ordinal, -1);
    s.integer(totk::hash(p + "ExtraLife"), item.ordinal, 0);
    if (s.count(p + "RecordExtraLife"))
      s.integer(totk::hash(p + "RecordExtraLife"), item.ordinal, 0);
  });
}
void Totk::fillStacks(Save &s, ItemGroup group) const {
  if (group != ItemGroup::Materials && group != ItemGroup::Arrows &&
      group != ItemGroup::Unknown)
    throw Error("This section does not support filling stacks.");
  auto all = items(s);
  auto h = totk::hash(pouch(group) + "Content.StockNum");
  columns(s, group, capacity(s, group));
  s.edit("Fill inventory stacks", [&] {
    for (auto &item : all)
      if (item.group == group && s.integer(h, item.ordinal) >= 0)
        s.integer(h, item.ordinal, 999);
  });
}
} // namespace zse
