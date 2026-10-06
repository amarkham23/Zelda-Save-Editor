// Copyright (c) 2026 StockiestThree9. MIT.
#include "totk.hpp"
#include <algorithm>
#include <set>
#include <tuple>

namespace zse {
namespace {
std::string actorId(const std::string &path) {
  if (path.rfind("Work/Actor/", 0) != 0)
    return {};
  auto end = path.find(".engine__actor__ActorParam.", 11);
  if (end == std::string::npos)
    return {};
  return path.substr(11, end - 11);
}
std::string text(const ByNode &row, const char *key) {
  auto n = row.get(key);
  return n && n->type == 0xa0 ? n->text : std::string{};
}
std::string part(const std::string &id) { return id.substr(id.rfind('_') + 1); }
std::string stars(int value) {
  return value == 0 ? "No upgrades"
                    : std::to_string(value) + (value == 1 ? " star" : " stars");
}
std::string join(const std::vector<std::string> &parts) {
  std::string result;
  for (auto &p : parts)
    if (!p.empty()) {
      if (!result.empty())
        result += " · ";
      result += p;
    }
  return result;
}
} // namespace
void readTotkItemTable(Metadata &metadata, const Bytes &bytes) {
  auto table = readByml(bytes);
  if (table.type != 0xc0)
    throw Error("Unexpected TOTK item table layout.");
  std::map<std::string, ItemVariant> result;
  for (auto &row : table.array) {
    auto id = text(row, "__RowId");
    if (id.empty() || id.size() >= 64)
      continue;
    ItemVariant info;
    info.family = id;
    info.category = text(row, "PouchCategory");
    info.armor = info.category == "Armor";
    info.conversion = text(row, "ConversionTargetActor");
    info.alias = text(row, "FakeActorName");
    auto performance = row.get("EquipmentPerformance");
    if (performance &&
        (performance->type == 0xd1 || performance->type == 0xd3) &&
        performance->number < 100000)
      info.performance = int(performance->number);
    if (info.armor) {
      auto rank = row.get("ArmorRank");
      info.stars = 0; // Omitted rank uses the game's base (unupgraded) value.
      if (rank && (rank->type == 0xd1 || rank->type == 0xd3) &&
          rank->number >= 1 && rank->number <= 5)
        info.stars = int(rank->number) - 1;
      info.nextArmor = actorId(text(row, "ArmorNextRankActor"));
    }
    result.emplace(id, std::move(info));
  }
  if (result.empty())
    throw Error("The TOTK item table is empty.");
  metadata.variants = std::move(result);
}
void readTotkActorTable(Metadata &metadata, const Bytes &bytes) {
  auto table = readByml(bytes);
  if (table.type != 0xc0)
    throw Error("Unexpected TOTK actor table layout.");
  for (auto &row : table.array) {
    auto it = metadata.variants.find(text(row, "__RowId"));
    if (it == metadata.variants.end())
      continue;
    auto &info = it->second;
    auto life = row.get("MaxLife");
    if (life && (life->type == 0xd1 || life->type == 0xd3) &&
        life->number > 0 && life->number <= 999999)
      info.durability = int(life->number);
    auto model = text(row, "FmdbName"), project = text(row, "ModelProjectName");
    if (!model.empty())
      info.model = model;
    if (!project.empty())
      info.modelProject = project;
  }
}
void linkTotkVariants(Metadata &metadata) {
  // Upgrade chains are explicit game data. Never infer a star level from the
  // numeric part of an actor ID: many armor families have irregular IDs.
  std::map<std::string, std::string> previous;
  std::map<std::string, std::vector<std::string>> parents;
  std::set<std::string> ambiguous;
  for (auto &pair : metadata.variants) {
    auto &info = pair.second;
    info.family = pair.first;
    info.style.clear();
    info.condition.clear();
    auto next = metadata.variants.find(info.nextArmor);
    if (!info.armor || next == metadata.variants.end() || !next->second.armor ||
        info.stars < 0 || next->second.stars != info.stars + 1 ||
        part(pair.first) != part(next->first))
      continue;
    parents[info.nextArmor].push_back(pair.first);
  }
  for (auto &pair : parents) {
    auto candidates = pair.second;
    if (candidates.size() > 1) {
      // Internal, unnamed actors can also point into a playable upgrade
      // chain. Prefer its unique matching display name when available.
      std::vector<std::string> named;
      auto target = metadata.names.find(pair.first);
      if (target != metadata.names.end())
        for (auto &id : candidates) {
          auto name = metadata.names.find(id);
          if (name != metadata.names.end() && name->second == target->second)
            named.push_back(id);
        }
      if (named.size() == 1)
        candidates = std::move(named);
    }
    previous.emplace(pair.first, candidates.front());
    if (candidates.size() != 1)
      ambiguous.insert(pair.first);
  }
  for (auto &pair : metadata.variants) {
    auto &info = pair.second;
    if (!info.armor)
      continue;
    std::string root = pair.first;
    std::set<std::string> visited;
    while (previous.count(root) && !ambiguous.count(root) &&
           visited.insert(root).second)
      root = previous.at(root);
    if (!previous.count(root) && metadata.variants.at(root).stars == 0)
      info.family = root;
  }
  // The hood conversion is explicitly paired at each rank by the game table.
  // The converted actor supplies FakeActorName pointing back to the hood-up
  // actor.
  for (auto &pair : metadata.variants) {
    auto &info = pair.second;
    if (!info.armor || info.alias.empty() || info.alias != info.conversion)
      continue;
    auto it = metadata.variants.find(info.conversion);
    if (it == metadata.variants.end())
      continue;
    auto &original = it->second;
    if (!original.armor || original.conversion != pair.first ||
        original.stars != info.stars || part(pair.first) != part(it->first))
      continue;
    if (original.family == "Armor_001_Head") {
      info.family = original.family;
      info.style = "Hood down";
      original.style = "Hood up";
    }
  }
  // Pristine weapon models are the matching pre-decay (_Before) model in the
  // same installed model project. Require an exact, unique counterpart.
  for (auto &pair : metadata.variants) {
    auto &pristine = pair.second;
    if (pristine.category != "Weapon" || pristine.model.size() < 7 ||
        pristine.model.compare(pristine.model.size() - 7, 7, "_Before") != 0 ||
        pristine.modelProject.empty())
      continue;
    auto base = pristine.model.substr(0, pristine.model.size() - 7);
    std::vector<std::string> matches;
    for (auto &other : metadata.variants)
      if (other.second.category == "Weapon" &&
          other.second.modelProject == pristine.modelProject &&
          other.second.model == base && other.second.alias.empty())
        matches.push_back(other.first);
    if (matches.size() != 1)
      continue;
    auto &decayed = metadata.variants.at(matches.front());
    pristine.family = decayed.family = pair.first;
    pristine.condition = "Pristine";
    decayed.condition = "Decayed";
  }
  // The wooden practice variants use an explicit display alias, the same
  // model and identical pouch performance. Keep those under Appearance.
  for (auto &pair : metadata.variants) {
    auto &alternate = pair.second;
    if (alternate.armor || alternate.alias.empty() || alternate.model.empty())
      continue;
    auto it = metadata.variants.find(alternate.alias);
    if (it == metadata.variants.end())
      continue;
    auto &standard = it->second;
    if (alternate.category != standard.category ||
        alternate.model != standard.model ||
        alternate.modelProject != standard.modelProject ||
        alternate.performance != standard.performance ||
        alternate.durability != standard.durability ||
        !standard.condition.empty())
      continue;
    alternate.family = standard.family;
    alternate.style = "Alternate appearance";
    standard.style = "Standard appearance";
  }
}
int Totk::initialDurability(const std::string &id) const {
  auto it = metadata.variants.find(id);
  return it == metadata.variants.end() ? 1 : std::max(1, it->second.durability);
}
std::vector<ItemFamily> Totk::families(const Save &s, ItemGroup group) const {
  std::map<std::string, ItemFamily> grouped;
  for (auto &id : catalog(s, group)) {
    auto found = metadata.variants.find(id);
    auto family = found == metadata.variants.end() ? id : found->second.family;
    if (family.empty())
      family = id;
    auto &entry = grouped[family];
    entry.ids.push_back(id);
  }
  std::vector<ItemFamily> result;
  for (auto &pair : grouped) {
    auto &entry = pair.second;
    std::sort(entry.ids.begin(), entry.ids.end(), [&](auto &a, auto &b) {
      auto value = [&](const std::string &id) {
        auto it = metadata.variants.find(id);
        if (it == metadata.variants.end())
          return std::make_tuple(0, 0, 0, id);
        auto &v = it->second;
        return std::make_tuple(std::max(0, v.stars),
                               v.style == "Hood down" ? 1 : 0,
                               v.condition == "Decayed" ? 1 : 0, id);
      };
      return value(a) < value(b);
    });
    entry.id = entry.ids.front();
    entry.name = metadata.itemName(entry.id);
    if (entry.ids.size() > 1) {
      std::set<int> ranks;
      std::set<std::string> styles, conditions;
      for (auto &id : entry.ids) {
        auto it = metadata.variants.find(id);
        if (it == metadata.variants.end())
          continue;
        auto &v = it->second;
        if (v.stars >= 0)
          ranks.insert(v.stars);
        if (!v.style.empty())
          styles.insert(v.style);
        if (!v.condition.empty())
          conditions.insert(v.condition);
      }
      std::vector<std::string> detail;
      if (ranks.size() > 1)
        detail.push_back("Choose upgrade level");
      if (styles.size() > 1)
        detail.push_back(styles.count("Hood up") ? "Hood up / down"
                                                 : "Choose appearance");
      if (conditions.size() > 1)
        detail.push_back("Pristine / decayed");
      entry.detail = join(detail);
    } else
      entry.detail = variantLabel(entry.id);
    result.push_back(std::move(entry));
  }
  std::sort(result.begin(), result.end(), [](auto &a, auto &b) {
    return std::tie(a.name, a.id) < std::tie(b.name, b.id);
  });
  return result;
}
std::string Totk::variantLabel(const std::string &id) const {
  auto it = metadata.variants.find(id);
  if (it == metadata.variants.end())
    return {};
  auto &v = it->second;
  return join(
      {v.stars >= 0 ? stars(v.stars) : std::string{}, v.style, v.condition});
}
std::vector<std::pair<std::string, std::string>>
Totk::variantOptions(const Save &s, ItemGroup group, const std::string &id,
                     VariantProperty property) const {
  auto it = metadata.variants.find(id);
  if (it == metadata.variants.end() || it->second.family.empty())
    return {};
  auto &selected = it->second;
  std::vector<std::pair<std::string, std::string>> result;
  std::set<std::string> labels;
  for (auto &candidate : catalog(s, group)) {
    auto found = metadata.variants.find(candidate);
    if (found == metadata.variants.end())
      continue;
    auto &v = found->second;
    if (v.family != selected.family)
      continue;
    if (property != VariantProperty::Stars && v.stars != selected.stars)
      continue;
    if (property != VariantProperty::Style && v.style != selected.style)
      continue;
    if (property != VariantProperty::Condition &&
        v.condition != selected.condition)
      continue;
    std::string label = property == VariantProperty::Stars
                            ? (v.stars < 0 ? "" : stars(v.stars))
                        : property == VariantProperty::Style ? v.style
                                                             : v.condition;
    if (label.empty())
      continue;
    // Ambiguous mappings are not offered as if they were interchangeable.
    if (!labels.insert(label).second)
      return {};
    result.push_back({candidate, label});
  }
  std::sort(result.begin(), result.end(), [&](auto &a, auto &b) {
    if (property == VariantProperty::Stars)
      return metadata.variants.at(a.first).stars <
             metadata.variants.at(b.first).stars;
    return a.second < b.second;
  });
  return result;
}
void Totk::changeVariant(Save &s, const Item &item,
                         const std::string &id) const {
  auto a = metadata.variants.find(item.id), b = metadata.variants.find(id);
  if (a == metadata.variants.end() || b == metadata.variants.end() ||
      a->second.family.empty() || a->second.family != b->second.family)
    throw Error("Choose a variant of this item.");
  replaceItem(s, item, id);
}
} // namespace zse
