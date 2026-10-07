// Copyright (c) 2026 StockiestThree9. MIT.
#include "totk.hpp"
#include <algorithm>
#include <cstdio>
namespace zse {
namespace {
const char *sections[] = {"Weapon",   "Bow",  "Arrow",   "Shield",      "Armor",
                          "Material", "Food", "KeyItem", "SpecialParts"};
std::string prefix(ItemGroup group) {
  return std::string("Pouch.") + sections[int(group)] + ".Content.";
}
bool equipment(ItemGroup group) {
  return group == ItemGroup::Weapons || group == ItemGroup::Bows || group == ItemGroup::Shields;
}
void options(EditField &field, std::initializer_list<const char *> values) {
  const std::map<std::string, std::string> names = {{"None", "None"},
                                                    {"AttackUp", "Attack up"},
                                                    {"AttackUpPlus", "Attack up +"},
                                                    {"DurabilityUp", "Durability up"},
                                                    {"DurabilityUpPlus", "Durability up +"},
                                                    {"FinishBlow", "Critical hit"},
                                                    {"LongThrow", "Long throw"},
                                                    {"RapidFire", "Quick shot"},
                                                    {"FiveWay", "Five-shot burst"},
                                                    {"GuardUp", "Shield guard up"},
                                                    {"GuardUpPlus", "Shield guard up +"}};
  for (auto value : values) {
    auto found = names.find(value);
    field.choices.push_back(
        {totk::hash(value), found == names.end() ? readable(value) : found->second});
  }
}
} // namespace
void Totk::definitions(const Save &s) {
  metadata.fields.clear();
  originalItems.clear();
  for (auto &item : items(Save(s.original())))
    originalItems.emplace(item.id, item.group);
  auto add = [&](const std::string &name) {
    auto h = totk::hash(name);
    auto it = s.types().find(h);
    if (it == s.types().end() || !s.count(h))
      return;
    auto t = it->second;
    Field f{name, h, Kind::Unknown, 1, bool(t & 1)};
    switch (t & ~1U) {
    case 0:
      f.kind = Kind::Bool;
      break;
    case 2:
      f.kind = Kind::Int;
      break;
    case 4:
      f.kind = Kind::Float;
      break;
    case 6:
    case 20:
      f.kind = Kind::UInt;
      break;
    case 8:
      f.kind = Kind::Vec2;
      f.width = 2;
      break;
    case 10:
      f.kind = Kind::Vec3;
      f.width = 3;
      break;
    case 12:
    case 14:
    case 16:
      f.kind = Kind::String;
      f.width = (t & ~1U) == 12 ? 4 : (t & ~1U) == 14 ? 8 : 16;
      break;
    case 26:
    case 28:
    case 30:
      f.kind = Kind::WideString;
      f.width = (t & ~1U) == 26 ? 8 : (t & ~1U) == 28 ? 16 : 32;
      break;
    default:
      return;
    }
    metadata.fields.emplace(h, f);
  };
  for (auto name : {"PlayerStatus.MaxLife", "PlayerStatus.Life", "PlayerStatus.CurrentRupee",
                    "PlayerStatus.MaxStamina", "PlayerStatus.MaxEnergy", "HorseInnMemberPoint",
                    "PlayerStatus.SavePos"})
    add(name);
  for (auto section : sections) {
    auto p = std::string("Pouch.") + section;
    add(p + ".ValidNum");
    for (auto property : {"Name", "Life", "StockNum", "Effect.Type", "Effect.Value",
                          "ColorVariation", "Combined.Name", "Combined.Life", "ExtraLife",
                          "RecordExtraLife", "LifeRecover", "Effect.Level", "Effect.Time", "Price"})
      add(p + ".Content." + property);
  }
  for (auto name :
       {"Sequence_CurrentBanc", "MapData.IconData.StampData.Type", "MapData.IconData.StampData.Pos",
        "MapData.IconData.StampData.Layer", "MapData.IconData.MapPinData.Type",
        "MapData.IconData.MapPinData.Pos", "AutoBuilder.Draft.Content.Index",
        "AutoBuilder.Draft.Content.IsFavorite", "AutoBuilder.Draft.Content.CameraPos",
        "AutoBuilder.Draft.Content.CameraAt", "AmiiboData.TouchLog.CharacterBaseID",
        "AmiiboData.TouchLog.CharacterID", "AmiiboData.TouchLog.NumberingID",
        "AmiiboData.TouchLog.Count", "AmiiboData.TouchLog.Day", "AmiiboData.TouchLog.Hour",
        "AmiiboData.TouchLog.Minute", "AmiiboData.TouchLog.Month", "AmiiboData.TouchLog.Year"})
    add(name);
  for (auto suffix :
       {"ActorName", "Name", "Mane", "Saddle", "Rein", "Familiarity", "IsFamiliarityChecked",
        "Toughness", "Speed", "ChargeNum", "HorsePower", "HorseType", "ColorType", "FootType",
        "RoomID", "Body.Pattern", "Body.EyeColor"})
    add(std::string("OwnedHorseList.") + suffix);
  for (auto &q : metadata.quests) {
    add("Step_" + q.first);
    for (auto &flag : q.second)
      add(flag);
  }
  for (auto &p : metadata.world) {
    add(p.field);
    if (p.category == "Koroks")
      add("KorokCarryProgress." + p.id);
  }
  for (auto &pair : metadata.names) {
    auto id = pair.first;
    for (auto prefix : {"IsGet.", "IsGetAnyway.", "EquipmentDeathCount.", "DefeatedEnemyNum.",
                        "PictureBook.State.", "PictureBookSize.", "IsRegisteredPictureBook."})
      add(std::string(prefix) + id);
  }
  for (unsigned i = 0; i < 152; ++i) {
    char name[80];
    snprintf(name, sizeof(name), "DungeonState.Dungeon%03u", i);
    add(name);
    snprintf(name, sizeof(name), "ArrivalPointState.CheckPoint%03u", i);
    add(name);
  }
  for (unsigned i = 1; i <= 15; ++i) {
    char name[80];
    snprintf(name, sizeof(name), "IsActivateCannon.Tower%02u", i);
    add(name);
  }
  for (auto &pair : metadata.names)
    for (auto suffix : {".State", ".IsNew"})
      add("PictureBookData." + pair.first + suffix);
  for (auto id :
       {"UltraHand", "OneTouchBond", "Tooreroof", "ReverseRecorder", "AutoBuilder", "AmiiboItem"}) {
    add(std::string("IsGet.Obj_") + id);
    add(std::string("IsGetAnyway.Obj_") + id);
  }
  for (auto &pair : s.types())
    if (!metadata.fields.count(pair.first) && s.count(pair.first)) {
      uint32_t t = pair.second & ~1U;
      Kind kind = Kind::Unknown;
      size_t width = 1;
      switch (t) {
      case 0:
        kind = Kind::Bool;
        break;
      case 2:
        kind = Kind::Int;
        break;
      case 4:
        kind = Kind::Float;
        break;
      case 6:
      case 20:
        kind = Kind::UInt;
        break;
      case 8:
        kind = Kind::Vec2;
        width = 2;
        break;
      case 10:
        kind = Kind::Vec3;
        width = 3;
        break;
      case 12:
      case 14:
      case 16:
        kind = Kind::String;
        width = t == 12 ? 4 : t == 14 ? 8 : 16;
        break;
      case 26:
      case 28:
      case 30:
        kind = Kind::WideString;
        width = t == 26 ? 8 : t == 28 ? 16 : 32;
        break;
      default:
        continue;
      }
      char name[16];
      snprintf(name, sizeof(name), "#%08X", pair.first);
      metadata.fields.emplace(pair.first,
                              Field{name, pair.first, kind, width, bool(pair.second & 1)});
    }
}
EditField Totk::field(const std::string &name, const std::string &label, size_t index) const {
  uint32_t hash = name.rfind("#", 0) == 0 ? uint32_t(std::stoul(name.substr(1), nullptr, 16))
                                          : totk::hash(name);
  auto i = metadata.fields.find(hash);
  if (i == metadata.fields.end())
    throw Error("That TOTK field is unavailable in this save.");
  EditField f;
  f.field = i->second;
  f.index = index;
  f.label = label;
  f.whole = f.field.kind != Kind::Float && f.field.kind != Kind::Vec2 && f.field.kind != Kind::Vec3;
  f.minimum = 0;
  f.maximum = 999999;
  if (name.find("DungeonState.") == 0)
    options(f, {"Hidden", "Appear", "Open", "Enter", "Clear"});
  if (name.find("ArrivalPointState.") == 0)
    options(f, {"Close", "Open"});
  if (name.find("ColorVariation") != std::string::npos)
    options(f, {"None", "Blue", "Red", "Yellow", "White", "Black", "Purple", "Green", "LightBlue",
                "Navy", "Orange", "Pink", "Crimson", "LightYellow", "Brown", "Gray"});
  if (name == "Pouch.Weapon.Content.Effect.Type")
    options(f, {"None", "AttackUp", "AttackUpPlus", "DurabilityUp", "DurabilityUpPlus",
                "FinishBlow", "LongThrow"});
  if (name == "Pouch.Bow.Content.Effect.Type")
    options(f, {"None", "AttackUp", "AttackUpPlus", "DurabilityUp", "DurabilityUpPlus", "RapidFire",
                "FiveWay"});
  if (name == "Pouch.Shield.Content.Effect.Type")
    options(f, {"None", "DurabilityUp", "DurabilityUpPlus", "GuardUp", "GuardUpPlus"});
  if (name == "Pouch.Food.Content.Effect.Type") {
    const std::pair<const char *, const char *> meals[] = {{"None", "None"},
                                                           {"LifeMaxUp", "Extra hearts"},
                                                           {"StaminaRecover", "Restore stamina"},
                                                           {"ExStaminaMaxUp", "Extra stamina"},
                                                           {"LifeRepair", "Repair gloom damage"},
                                                           {"ResistHot", "Heat resistance"},
                                                           {"ResistCold", "Cold resistance"},
                                                           {"ResistElectric", "Shock resistance"},
                                                           {"ResistBurn", "Flame resistance"},
                                                           {"AttackUp", "Attack up"},
                                                           {"DefenseUp", "Defense up"},
                                                           {"AllSpeed", "Movement speed up"},
                                                           {"QuietnessUp", "Stealth up"},
                                                           {"MiasmaGuard", "Gloom resistance"},
                                                           {"NotSlippy", "Slip resistance"},
                                                           {"LightEmission", "Glow"}};
    for (auto &effect : meals)
      f.choices.push_back({totk::hash(effect.first), effect.second});
  }
  if (f.choices.empty()) {
    auto it = metadata.enums.find(f.field.hash);
    if (it != metadata.enums.end())
      f.choices = it->second;
  }
  return f;
}
std::vector<EditField> Totk::stats(const Save &s) const {
  struct Definition {
    const char *key;
    const char *label;
    const char *unit;
    double scale, min, max;
    bool whole;
  };
  const Definition defs[] = {
      {"PlayerStatus.CurrentRupee", "Rupees", "rupees", 1, 0, 999999, true},
      {"PlayerStatus.MaxLife", "Maximum hearts", "hearts", 4, 3, 40, true},
      {"PlayerStatus.Life", "Current hearts", "hearts", 4, 0, 40, false},
      {"PlayerStatus.MaxStamina", "Maximum stamina wheels", "wheels", 1000, 1, 3, false},
      {"PlayerStatus.MaxEnergy", "Battery cells", "cells", 1000, 3, 48, true},
      {"Pouch.Weapon.ValidNum", "Weapon slots", "", 1, 8, 20, true},
      {"Pouch.Bow.ValidNum", "Bow slots", "", 1, 5, 14, true},
      {"Pouch.Shield.ValidNum", "Shield slots", "", 1, 4, 20, true},
      {"HorseInnMemberPoint", "Pony Points", "points", 1, 0, 999999, true}};
  std::vector<EditField> result;
  for (auto &d : defs)
    if (s.count(d.key)) {
      auto f = field(d.key, d.label);
      f.unit = d.unit;
      f.scale = d.scale;
      f.minimum = d.min;
      f.maximum = d.max;
      f.whole = d.whole;
      if (std::string(d.key) == "PlayerStatus.MaxEnergy")
        f.description = "3 cells per battery · 48 cells fills both rows";
      result.push_back(f);
    }
  return result;
}
std::vector<Item> Totk::items(const Save &s) const {
  std::vector<Item> result;
  for (unsigned group = 0; group < 9; ++group) {
    auto key = prefix(ItemGroup(group)) + "Name";
    auto h = totk::hash(key);
    if (!s.count(h))
      continue;
    for (size_t i = 0; i < capacity(s, ItemGroup(group)); ++i) {
      auto name = s.string(h, 64, i);
      if (!name.empty())
        result.push_back({name, group * 4096 + i, i, ItemGroup(group)});
    }
  }
  return result;
}
std::vector<EditField> Totk::itemFields(const Save &s, const Item &item) const {
  std::vector<EditField> result;
  auto p = prefix(item.group);
  auto add = [&](const char *suffix, const char *label, const char *unit = "", double scale = 1) {
    auto key = p + suffix;
    if (!s.count(key) || item.ordinal >= s.count(totk::hash(key)))
      return;
    auto f = field(key, label, item.ordinal);
    f.unit = unit;
    f.scale = scale;
    f.whole = true;
    result.push_back(f);
  };
  if (equipment(item.group)) {
    add("Life", "Durability", "durability");
    add("Effect.Type", "Equipment bonus");
    if (s.count(p + "Effect.Type") > item.ordinal &&
        s.word(totk::hash(p + "Effect.Type"), item.ordinal) != totk::hash("None"))
      add("Effect.Value", "Bonus strength");
    bool fused = s.count(p + "Combined.Name") / 16 > item.ordinal &&
                 !s.string(totk::hash(p + "Combined.Name"), 64, item.ordinal).empty();
    if (fused) {
      add("Combined.Life", "Fused item durability", "durability");
      add("ExtraLife", "Remaining fuse bonus", "durability");
      add("RecordExtraLife", "Maximum fuse bonus", "durability");
    }
  } else if (item.group == ItemGroup::Armor)
    add("ColorVariation", "Dye color");
  else {
    if (s.count(p + "StockNum") > item.ordinal &&
        s.integer(totk::hash(p + "StockNum"), item.ordinal) >= 0)
      add("StockNum", "Quantity");
    if (!result.empty())
      result.front().maximum = 999;
    if (item.group == ItemGroup::Food) {
      auto before = result.size();
      add("LifeRecover", "Hearts restored", "hearts", 4);
      if (result.size() > before)
        result.back().whole = false;
      add("Effect.Type", "Meal effect");
      add("Effect.Level", "Effect strength");
      add("Effect.Time", "Effect duration", "seconds");
      add("Price", "Sell value", "rupees");
    }
  }
  return result;
}
std::string Totk::itemValue(const Save &s, const Item &item) const {
  auto fields = itemFields(s, item);
  if (fields.empty())
    return "Owned";
  auto f = fields.front();
  auto shown = displayValue(s, f);
  if (equipment(item.group))
    return shown + " durability";
  if (item.group == ItemGroup::Armor)
    return shown;
  return "× " + shown;
}
std::string Totk::itemDetail(const Save &s, const Item &item) const {
  auto detail = variantLabel(item.id);
  auto key = prefix(item.group) + "Combined.Name";
  if (s.count(key) / 16 > item.ordinal) {
    auto id = s.string(totk::hash(key), 64, item.ordinal);
    if (!id.empty()) {
      if (!detail.empty())
        detail += " · ";
      detail += "Fused: " + metadata.itemName(id);
    }
  }
  if (equipped(s, item)) {
    if (!detail.empty())
      detail += " · ";
    detail += "Equipped";
  }
  return detail.empty()
             ? (item.group == ItemGroup::Unknown ? "Zonai devices" : groupName(item.group))
             : detail;
}
std::vector<EditField> Totk::matching(const Save &s,
                                      const std::function<bool(const Field &)> &predicate) const {
  std::vector<EditField> result;
  for (auto &pair : metadata.fields) {
    auto &f = pair.second;
    if (!s.count(f.hash) || !predicate(f))
      continue;
    // Preserve IDs and enum fields that lack a defined choice list in this build.
    auto e = field(f.name, readable(f.name));
    if (((s.types().at(f.hash) & ~1U) == 6 && e.choices.empty()))
      continue;
    e.description = f.name;
    if (f.name.rfind("PictureBookData.", 0) == 0 && f.name.size() > 22) {
      auto id = f.name.substr(16, f.name.size() - 22);
      e.label = metadata.itemName(id);
      for (auto &c : e.choices) {
        if (c.first == totk::hash("Buy"))
          c.second = "Stock picture";
        else if (c.first == totk::hash("TakePhoto"))
          c.second = "Your photograph";
        else if (c.first == totk::hash("Unopened"))
          c.second = "Missing";
      }
    }
    if (f.kind == Kind::Int) {
      e.minimum = -2147483648.0;
      e.maximum = 2147483647.0;
    }
    if (f.kind == Kind::UInt)
      e.maximum = 4294967295.0;
    if (f.kind == Kind::Float) {
      e.minimum = -1e30;
      e.maximum = 1e30;
    }
    if (f.kind == Kind::Vec3) {
      e.minimum = -100000;
      e.maximum = 100000;
    }
    result.push_back(e);
  }
  std::sort(result.begin(), result.end(),
            [](auto &a, auto &b) { return a.field.name < b.field.name; });
  return result;
}
std::vector<EditField> Totk::elements(const Save &s, const Field &f) const {
  std::vector<EditField> result;
  size_t count = (f.kind == Kind::String || f.kind == Kind::WideString) ? s.count(f.hash) / f.width
                 : f.kind == Kind::Bool                                 ? s.logicalCount(f.hash)
                                                                        : s.count(f.hash);
  for (size_t i = 0; i < count; ++i) {
    auto e = field(f.name, "Entry " + std::to_string(i + 1), i);
    if (f.kind == Kind::Vec3) {
      e.label =
          std::array<const char *, 3>{"East / west (X)", "Height (Y)", "North / south (Z)"}[i % 3];
      e.minimum = -100000;
      e.maximum = 100000;
    }
    result.push_back(e);
  }
  return result;
}
} // namespace zse
