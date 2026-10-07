// Copyright (c) 2026 StockiestThree9. MIT.
#include "totk.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <set>
namespace zse {
namespace {
std::string pouch(ItemGroup g) {
  if (g != ItemGroup::Weapons && g != ItemGroup::Bows && g != ItemGroup::Shields)
    throw Error("Choose equipment.");
  return std::string("Pouch.") +
         (g == ItemGroup::Weapons ? "Weapon"
          : g == ItemGroup::Bows  ? "Bow"
                                  : "Shield") +
         ".Content.";
}
void currentItem(const Save &s, const Item &i) {
  auto key = pouch(i.group) + "Name";
  if (s.string(totk::hash(key), 64, i.ordinal) != i.id)
    throw Error("That item changed. Select it again.");
}
void put(Bytes &b, uint32_t v) {
  for (unsigned i = 0; i < 4; ++i)
    b.push_back(uint8_t(v >> (8 * i)));
}
uint32_t get(const Bytes &b, size_t p) {
  if (p > b.size() || b.size() - p < 4)
    throw Error("Incomplete blueprint file.");
  return uint32_t(b[p]) | (uint32_t(b[p + 1]) << 8) | (uint32_t(b[p + 2]) << 16) |
         (uint32_t(b[p + 3]) << 24);
}
uint32_t checksum(const Bytes &b, size_t count) {
  return hashName(std::string(b.begin(), b.begin() + count));
}
} // namespace
std::vector<std::string> Totk::fusionCatalog(const Save &s) const {
  std::set<std::string> result;
  for (auto &p : metadata.attachments)
    if (metadata.names.count(p.first))
      result.insert(p.first);
  for (auto &item : items(s))
    if (item.group == ItemGroup::Weapons || item.group == ItemGroup::Shields) {
      auto key = pouch(item.group) + "Combined.Name";
      auto id = s.string(totk::hash(key), 64, item.ordinal);
      if (!id.empty())
        result.insert(id);
    }
  return {result.begin(), result.end()};
}
void Totk::setFusion(Save &s, const Item &item, const std::string &id) const {
  if (item.ordinal >= capacity(s, item.group))
    throw Error("That item is outside the ordinary pouch.");
  currentItem(s, item);
  if (item.group != ItemGroup::Weapons && item.group != ItemGroup::Shields)
    throw Error("This item cannot be fused.");
  auto choices = fusionCatalog(s);
  if (std::find(choices.begin(), choices.end(), id) == choices.end())
    throw Error("Choose a verified attachment.");
  auto p = pouch(item.group);
  if (s.count(p + "Combined.Life") <= item.ordinal || s.count(p + "ExtraLife") <= item.ordinal)
    throw Error("This fusion layout is unsupported.");
  auto v = metadata.variants.find(id);
  int life = v == metadata.variants.end() || v->second.durability <= 0 ? -1 : v->second.durability;
  s.edit("Change fused attachment", [&] {
    s.string(totk::hash(p + "Combined.Name"), 64, item.ordinal, id);
    s.integer(totk::hash(p + "Combined.Life"), item.ordinal, life);
  });
}
void Totk::repairEquipment(Save &s, ItemGroup group) const {
  auto p = pouch(group);
  auto all = items(s);
  size_t repaired = 0;
  s.edit("Repair " + groupName(group), [&] {
    for (auto &item : all)
      if (item.group == group) {
        auto v = metadata.variants.find(item.id);
        if (v == metadata.variants.end() || v->second.durability <= 0)
          continue;
        int life = v->second.durability;
        auto effect = s.word(totk::hash(p + "Effect.Type"), item.ordinal);
        if (effect == totk::hash("DurabilityUp") || effect == totk::hash("DurabilityUpPlus"))
          life += std::max(0, s.integer(totk::hash(p + "Effect.Value"), item.ordinal));
        s.integer(totk::hash(p + "Life"), item.ordinal, life);
        if (group != ItemGroup::Bows) {
          auto id = s.string(totk::hash(p + "Combined.Name"), 64, item.ordinal);
          if (!id.empty()) {
            auto attachment = metadata.variants.find(id);
            if (attachment != metadata.variants.end() && attachment->second.durability > 0)
              s.integer(totk::hash(p + "Combined.Life"), item.ordinal,
                        attachment->second.durability);
            if (s.count(p + "RecordExtraLife") > item.ordinal)
              s.integer(totk::hash(p + "ExtraLife"), item.ordinal,
                        std::max(0, s.integer(totk::hash(p + "RecordExtraLife"), item.ordinal)));
          }
        }
        ++repaired;
      }
  });
  if (!repaired)
    throw Error("No equipment has verified game durability. Reload Game resources.");
}
void Totk::upgradeArmor(Save &s) const {
  auto all = items(s);
  std::vector<std::pair<Item, std::string>> replacements;
  for (auto &item : all)
    if (item.group == ItemGroup::Armor) {
      auto options = variantOptions(s, item.group, item.id, VariantProperty::Stars);
      int best = -1;
      std::string id;
      for (auto &option : options) {
        auto v = metadata.variants.find(option.first);
        if (v != metadata.variants.end() && v->second.stars > best) {
          best = v->second.stars;
          id = option.first;
        }
      }
      if (!id.empty() && id != item.id)
        replacements.emplace_back(item, id);
    }
  s.edit("Upgrade owned armor", [&] {
    for (auto &pair : replacements)
      s.string(totk::hash("Pouch.Armor.Content.Name"), 64, pair.first.ordinal, pair.second);
  });
}
void Totk::restorePristine(Save &s) const {
  auto all = items(s);
  std::vector<std::pair<Item, std::string>> replacements;
  for (auto &item : all)
    if (item.group == ItemGroup::Weapons && canManage(item))
      for (auto &choice : variantOptions(s, item.group, item.id, VariantProperty::Condition))
        if (choice.second == "Pristine" && choice.first != item.id)
          replacements.emplace_back(item, choice.first);
  s.edit("Restore pristine weapons", [&] {
    for (auto &pair : replacements)
      s.string(totk::hash("Pouch.Weapon.Content.Name"), 64, pair.first.ordinal, pair.second);
  });
}
void Totk::unlockPristine(Save &s) const {
  std::vector<uint32_t> flags;
  for (auto &pair : metadata.variants)
    if (pair.second.condition == "Decayed") {
      auto h = totk::hash("EquipmentDeathCount." + pair.first);
      if (s.count(h) == 1 && s.types().at(h) == 2)
        flags.push_back(h);
    }
  if (flags.empty())
    throw Error("Pristine weapon unlock fields are unavailable.");
  s.edit("Unlock pristine weapon spawns", [&] {
    for (auto h : flags)
      if (s.integer(h) < 1)
        s.integer(h, 0, 1);
  });
}
std::vector<EditField> Totk::horseFields(const Save &s, size_t slot) const {
  if (slot >= s.logicalCount(totk::hash("OwnedHorseList.ActorName")))
    throw Error("Horse slot is unavailable.");
  std::vector<EditField> out;
  struct Spec {
    const char *name;
    const char *label;
    double min, max, scale;
  };
  const Spec specs[] = {{"Name", "Name", 0, 0, 1},
                        {"Familiarity", "Bond (%)", 0, 100, 0.01},
                        {"Toughness", "Strength (health)", 100, 350, 1},
                        {"Speed", "Speed (stars)", 1, 5, 1},
                        {"ChargeNum", "Stamina (spurs)", 0, 5, 1},
                        {"HorsePower", "Pull (stars)", 1, 5, 1},
                        {"Mane", "Mane", 0, 0, 1},
                        {"Saddle", "Saddle", 0, 0, 1},
                        {"Rein", "Bridle", 0, 0, 1}};
  for (auto &spec : specs) {
    auto name = std::string("OwnedHorseList.") + spec.name;
    auto it = metadata.fields.find(totk::hash(name));
    if (it == metadata.fields.end() || slot >= s.logicalCount(it->first))
      continue;
    auto f = field(name, spec.label, slot);
    f.scale = spec.scale;
    f.minimum = spec.min;
    f.maximum = spec.max;
    f.whole = std::string(spec.name) != "Familiarity";
    if (std::string(spec.name) == "Speed" || std::string(spec.name) == "HorsePower")
      for (uint32_t n = 1; n <= 4; ++n)
        f.choices.emplace_back(n, std::to_string(n + 1) + " stars");
    if (std::string(spec.name) == "ChargeNum") {
      f.choices = {
          {0, "Unlimited"}, {2, "2 spurs"}, {3, "3 spurs"}, {4, "4 spurs"}, {5, "5 spurs"}};
    }
    if (std::string(spec.name) == "Saddle" || std::string(spec.name) == "Rein")
      for (auto &option : f.choices) {
        for (auto &pair : metadata.names)
          if (totk::hash(pair.first) == option.first) {
            option.second = pair.second;
            break;
          }
      }
    if (f.field.kind == Kind::UInt && f.choices.empty())
      continue;
    out.push_back(f);
  }
  return out;
}
std::vector<EditField> Totk::mapFields(const Save &s, size_t index) const {
  std::vector<EditField> out;
  for (auto suffix : {"Type", "Layer"}) {
    auto name = std::string("MapData.IconData.StampData.") + suffix;
    if (index < s.logicalCount(totk::hash(name))) {
      auto f = field(name, std::string(suffix) == "Type" ? "Icon" : "Map layer", index);
      if (!f.choices.empty())
        out.push_back(f);
    }
  }
  auto name = "MapData.IconData.StampData.Pos";
  if (index < s.logicalCount(totk::hash(name)))
    for (size_t axis = 0; axis < 2; ++axis) {
      auto f = field(name, axis ? "North / south (Z)" : "East / west (X)", index * 2 + axis);
      f.minimum = -100000;
      f.maximum = 100000;
      f.whole = false;
      out.push_back(f);
    }
  return out;
}
void Totk::clearMapStamps(Save &s) const {
  auto h = totk::hash("MapData.IconData.StampData.Type");
  auto it = metadata.enums.find(h);
  if (!s.count(h) || it == metadata.enums.end() ||
      std::none_of(it->second.begin(), it->second.end(),
                   [](auto &c) { return c.first == totk::hash("Invalid"); }))
    throw Error("Map stamp layout is unavailable.");
  s.edit("Clear map stamps", [&] {
    for (size_t i = 0; i < s.count(h); ++i)
      s.word(h, i, totk::hash("Invalid"));
  });
}
void Totk::resetAmiibo(Save &s) const {
  std::vector<uint32_t> dates;
  for (auto part : {"Day", "Hour", "Minute", "Month", "Year"}) {
    auto h = totk::hash(std::string("AmiiboData.TouchLog.") + part);
    if (s.count(h))
      dates.push_back(h);
  }
  if (dates.size() != 5)
    throw Error("Daily amiibo history is unavailable in this save.");
  s.edit("Reset daily amiibo scans", [&] {
    for (auto h : dates)
      for (size_t i = 0; i < s.count(h); ++i)
        s.integer(h, i, 0);
  });
}
std::vector<EditField> Totk::questFields(const Save &s, const std::string &id) const {
  std::vector<EditField> out;
  auto stage = "Step_" + id;
  if (s.count(stage)) {
    auto f = field(stage, "Quest stage");
    f.description = "Related story and reward flags remain separate";
    if (!f.choices.empty())
      out.push_back(f);
  }
  auto it = metadata.quests.find(id);
  if (it == metadata.quests.end())
    return out;
  for (auto &name : it->second)
    if (s.count(name)) {
      auto f = field(name, readable(name.substr(id.size() + 1)));
      f.description = "Direct quest flag · related story flags may need matching changes";
      if (f.field.kind == Kind::Bool || !f.choices.empty())
        out.push_back(f);
    }
  return out;
}
Blueprint Totk::blueprint(const Save &s, size_t slot) const {
  Blueprint b;
  b.slot = slot;
  auto index = totk::hash("AutoBuilder.Draft.Content.Index"),
       favorite = totk::hash("AutoBuilder.Draft.Content.IsFavorite");
  if (slot >= s.logicalCount(index))
    throw Error("That Autobuild slot is unavailable.");
  b.index = s.integer(index, slot);
  b.favorite = s.boolean(favorite, slot);
  b.data = s.binary(totk::hash("AutoBuilder.Draft.Content.CombinedActorInfo"), slot);
  for (size_t i = 0; i < 6; ++i)
    b.camera[i] = s.word(totk::hash(i < 3 ? "AutoBuilder.Draft.Content.CameraPos"
                                          : "AutoBuilder.Draft.Content.CameraAt"),
                         slot * 3 + i % 3);
  return b;
}
Bytes Totk::exportBlueprint(const Save &s, size_t slot) const {
  auto b = blueprint(s, slot);
  if (b.index < 0)
    throw Error("There is no build in this slot to export.");
  Bytes out = {'Z', 'S', 'B', 'P'};
  put(out, 1);
  put(out, uint32_t(b.data.size()));
  put(out, uint32_t(b.index));
  for (auto word : b.camera)
    put(out, word);
  out.insert(out.end(), b.data.begin(), b.data.end());
  put(out, checksum(out, out.size()));
  return out;
}
void Totk::importBlueprint(Save &s, size_t slot, const Bytes &file) const {
  auto current = blueprint(s, slot);
  if (file.size() < 44 || std::memcmp(file.data(), "ZSBP", 4) || get(file, 4) != 1)
    throw Error("Choose a Zelda Save Editor .zsbp blueprint.");
  auto size = get(file, 8);
  if (size != current.data.size() || file.size() != 44 + size ||
      get(file, file.size() - 4) != checksum(file, file.size() - 4))
    throw Error("Blueprint size or checksum is invalid.");
  // The payload remains fixed in size; reject invalid camera floats before editing.
  for (size_t i = 0; i < 6; ++i) {
    uint32_t word = get(file, 16 + i * 4);
    float value;
    std::memcpy(&value, &word, 4);
    if (!std::isfinite(value))
      throw Error("Blueprint camera is invalid.");
  }
  Bytes data(file.begin() + 40, file.end() - 4);
  auto indexHash = totk::hash("AutoBuilder.Draft.Content.Index");
  int32_t newIndex = current.index;
  if (newIndex < 0) {
    newIndex = 0;
    for (size_t i = 0; i < s.count(indexHash); ++i) {
      auto old = s.integer(indexHash, i);
      if (old >= 1000000)
        throw Error("Autobuild history order is invalid.");
      newIndex = std::max(newIndex, old + 1);
    }
    if (newIndex > 1000000)
      throw Error("Autobuild history order is invalid.");
  }
  s.edit("Import Autobuild blueprint", [&] {
    s.binary(totk::hash("AutoBuilder.Draft.Content.CombinedActorInfo"), slot, data);
    s.integer(indexHash, slot, newIndex);
    for (size_t i = 0; i < 6; ++i)
      s.word(totk::hash(i < 3 ? "AutoBuilder.Draft.Content.CameraPos"
                              : "AutoBuilder.Draft.Content.CameraAt"),
             slot * 3 + i % 3, get(file, 16 + i * 4));
  });
}
void Totk::favoriteBlueprint(Save &s, size_t slot) const {
  auto b = blueprint(s, slot);
  auto h = totk::hash("AutoBuilder.Draft.Content.IsFavorite");
  size_t favorites = 0;
  for (size_t i = 0; i < s.logicalCount(h); ++i)
    favorites += s.boolean(h, i);
  if (b.index < 0)
    throw Error("Import a build before favoriting this slot.");
  if (!b.favorite && favorites >= 8)
    throw Error("Autobuild has eight favorites. Unfavorite one first.");
  s.edit(b.favorite ? "Unfavorite blueprint" : "Favorite blueprint",
         [&] { s.boolean(h, slot, !b.favorite); });
}
std::vector<EditField> Totk::abilities(const Save &s) const {
  const std::pair<const char *, const char *> names[] = {
      {"UltraHand", "Ultrahand"},    {"OneTouchBond", "Fuse"},     {"Tooreroof", "Ascend"},
      {"ReverseRecorder", "Recall"}, {"AutoBuilder", "Autobuild"}, {"AmiiboItem", "Amiibo"}};
  std::vector<EditField> out;
  for (auto &name : names) {
    auto key = std::string("IsGet.Obj_") + name.first,
         other = std::string("IsGetAnyway.Obj_") + name.first;
    if (s.count(key) != 1 || s.count(other) != 1 || s.types().at(totk::hash(key)) != 0 ||
        s.types().at(totk::hash(other)) != 0)
      continue;
    auto f = field(key, name.second);
    f.description = "Availability - associated story quests remain separate";
    out.push_back(f);
  }
  return out;
}
void Totk::setAbility(Save &s, const std::string &id, bool enabled) const {
  auto all = abilities(s);
  auto name = "IsGet.Obj_" + id;
  auto it = std::find_if(all.begin(), all.end(), [&](auto &f) { return f.field.name == name; });
  if (it == all.end())
    throw Error("That ability is unavailable in this save.");
  s.edit(it->label, [&] {
    s.boolean(it->field.hash, 0, enabled);
    s.boolean(totk::hash("IsGetAnyway.Obj_" + id), 0, enabled);
  });
}

} // namespace zse
