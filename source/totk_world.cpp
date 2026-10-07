// Copyright (c) 2026 StockiestThree9. MIT.
#include "totk.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <set>
namespace zse {
namespace {
float number(const ByNode &n) {
  float v = 0;
  if (n.type == 0xd2) {
    auto w = n.number;
    std::memcpy(&v, &w, 4);
  } else if (n.type == 0xd1)
    v = float(int32_t(n.number));
  else if (n.type == 0xd3)
    v = float(n.number);
  else
    throw Error("Invalid map coordinate.");
  if (!std::isfinite(v) || std::abs(v) > 100000)
    throw Error("Invalid map coordinate.");
  return v;
}
std::array<float, 3> position(const ByNode &n) {
  if (n.array.size() == 3)
    return {number(n.array[0]), number(n.array[1]), number(n.array[2])};
  auto x = n.get("X"), y = n.get("Y"), z = n.get("Z");
  if (!x || !y || !z)
    throw Error("Missing map coordinates.");
  return {number(*x), number(*y), number(*z)};
}
std::string stem(std::string path) {
  auto slash = path.find_last_of('/');
  if (slash != std::string::npos)
    path.erase(0, slash + 1);
  return path.substr(0, path.find('.'));
}
} // namespace
void readTotkWorld(Metadata &data, const Bytes &bytes, bool koroks) {
  auto root = readByml(bytes);
  if (root.type != 0xc1)
    throw Error("Invalid TOTK world table.");
  std::vector<WorldEntry> next = data.world;
  std::set<std::pair<std::string, std::string>> seen;
  for (auto &p : next)
    seen.emplace(p.category, p.id);
  auto add = [&](WorldEntry e) {
    if (next.size() >= 5000)
      throw Error("TOTK world catalogue is too large.");
    if (seen.emplace(e.category, e.id).second)
      next.push_back(std::move(e));
  };
  if (koroks) {
    for (auto &group : root.dict)
      for (auto &row : group.second.dict) {
        size_t end = 0;
        uint64_t id = 0;
        try {
          id = std::stoull(row.first, &end);
        } catch (...) {
          throw Error("Invalid Korok identity.");
        }
        if (!id || end != row.first.size())
          throw Error("Invalid Korok identity.");
        add({"Koroks", row.first, "IsAppearKorok." + row.first, position(row.second)});
      }
  } else {
    for (auto &group : root.dict) {
      std::string category = group.first == "Shrine"                     ? "Shrines"
                             : group.first == "CheckPoint"               ? "Lightroots"
                             : group.first == "Tower"                    ? "Towers"
                             : group.first == "CaveEntranceWell"         ? "Wells"
                             : group.first.rfind("CaveEntrance", 0) == 0 ? "Caves"
                                                                         : "Locations";
      if (group.second.type != 0xc0)
        continue;
      for (auto &row : group.second.array) {
        auto currentCategory = category;
        auto name = row.get("LocationName"), coords = row.get("Trans");
        if (!name || name->type != 0xa0 || !coords || coords->array.empty())
          continue;
        auto id = stem(name->text);
        if (id.empty())
          continue;
        std::string key = category == "Shrines"      ? "DungeonState."
                          : category == "Lightroots" ? "ArrivalPointState."
                          : category == "Towers"     ? "IsActivateCannon."
                                                     : "IsVisitLocation.";
        if (id.find("DeepHole") != std::string::npos)
          currentCategory = "Chasms";
        add({currentCategory, id, key + id, position(coords->array.front())});
      }
    }
  }
  data.world = std::move(next);
}
void readTotkPlacement(Metadata &data, const Bytes &bytes) {
  auto root = readByml(bytes);
  auto actors = root.get("Actors");
  if (!actors)
    return;
  for (auto &actor : actors->array) {
    auto kind = actor.get("Gyaml"), dynamic = actor.get("Dynamic"), coords = actor.get("Translate");
    if (!kind || !dynamic || !coords || kind->text.rfind("Enemy_CaveMaster", 0) != 0)
      continue;
    auto name = dynamic->get("LocationName");
    if (!name || name->text.empty() || name->text.size() > 127)
      continue;
    if (std::any_of(data.world.begin(), data.world.end(),
                    [&](auto &p) { return p.category == "Bubbulfrogs" && p.id == name->text; }))
      continue;
    if (data.world.size() >= 5000)
      throw Error("World records exceed the limit.");
    data.world.push_back(
        {"Bubbulfrogs", name->text, "IsGetCaveMasterMedal." + name->text, position(*coords)});
  }
}
std::vector<EditField> Totk::worldFields(const Save &s, const std::string &category) const {
  std::vector<EditField> out;
  for (auto &p : metadata.world)
    if (p.category == category) {
      auto name = p.field;
      if (category == "Koroks" && s.count("KorokCarryProgress." + p.id))
        name = "KorokCarryProgress." + p.id;
      if (!s.count(name))
        continue;
      auto f = field(name, category == "Koroks"
                               ? (name.rfind("KorokCarry", 0) == 0 ? "Korok reunion" : "Korok")
                           : metadata.locations.count(p.id) ? metadata.locations.at(p.id)
                                                            : readable(p.id));
      f.description = "X " + std::to_string(int(p.position[0])) + " · Z " +
                      std::to_string(int(p.position[2])) + " · Height " +
                      std::to_string(int(p.position[1]));
      out.push_back(f);
    }
  return out;
}
void Totk::completeWorld(Save &s, const std::string &category) const {
  auto all = worldFields(s, category);
  if (all.empty())
    throw Error("No editable records are available for this category.");
  std::vector<std::pair<EditField, uint32_t>> changes;
  for (auto &f : all) {
    if (f.field.kind == Kind::Bool)
      changes.emplace_back(f, 1);
    else {
      auto state = category == "Shrines" ? "Clear" : category == "Koroks" ? "Clear" : "Open";
      auto value = totk::hash(state);
      if (std::any_of(f.choices.begin(), f.choices.end(),
                      [&](auto &c) { return c.first == value; }))
        changes.emplace_back(f, value);
    }
  }
  s.edit("Complete " + category, [&] {
    for (auto &c : changes) {
      if (c.first.field.kind == Kind::Bool)
        s.boolean(c.first.field.hash, 0, true);
      else
        s.word(c.first.field.hash, 0, c.second);
    }
  });
}
void Totk::unlockCompendium(Save &s) const {
  auto all = matching(s, [](auto &f) {
    return f.name.rfind("PictureBookData.", 0) == 0 && f.name.size() > 6 &&
           f.name.compare(f.name.size() - 6, 6, ".State") == 0;
  });
  if (all.empty())
    throw Error("Compendium records are unavailable.");
  s.edit("Unlock missing compendium entries", [&] {
    for (auto &f : all)
      if (s.word(f.field.hash) == totk::hash("Unopened"))
        s.word(f.field.hash, 0, totk::hash("Buy"));
  });
}
} // namespace zse
