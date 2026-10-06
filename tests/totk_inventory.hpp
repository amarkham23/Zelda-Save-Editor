// Copyright (c) 2026 StockiestThree9. MIT.
// Real-fixture inventory tests. Never writes the input fixture or console
// saves.
static void testInventory(const Bytes &original) {
  Metadata metadata;
  Totk model(metadata);
  Save save(original);
  model.definitions(save);
  const char *groups[] = {"Weapon",   "Bow",  "Arrow",   "Shield",      "Armor",
                          "Material", "Food", "KeyItem", "SpecialParts"};
  const size_t capacities[] = {20, 14, 1, 20, 150, 255, 60, 100, 30};
  auto normalOnly = [&](const Bytes &before, ItemGroup group) {
    std::set<size_t> allowed;
    auto p = std::string("Pouch.") + groups[int(group)] + ".";
    const char *properties[] = {"Name",          "Life",
                                "StockNum",      "Effect.Type",
                                "Effect.Value",  "ColorVariation",
                                "Combined.Name", "Combined.Life",
                                "ExtraLife",     "RecordExtraLife",
                                "LifeRecover",   "Effect.Level",
                                "Effect.Time",   "Price",
                                "GetOrder",      "UseOrder",
                                "MaterialName"};
    for (auto property : properties) {
      auto h = totk::hash(p + "Content." + property);
      for (size_t i = 0; i < save.count(h) / 2; ++i) {
        auto at = save.fields().at(h)[i];
        for (size_t b = 0; b < 4; ++b)
          allowed.insert(at + b);
      }
    }
    for (auto suffix : {"EquipIndex", "EquipIndexes.Head", "EquipIndexes.Upper",
                        "EquipIndexes.Bottom"}) {
      auto h = totk::hash(p + suffix);
      if (save.count(h)) {
        auto at = save.fields().at(h)[0];
        for (size_t b = 0; b < 4; ++b)
          allowed.insert(at + b);
      }
    }
    bool okay = true;
    for (size_t i = 0; i < before.size(); ++i)
      if (before[i] != save.bytes()[i] && !allowed.count(i)) {
        okay = false;
        break;
      }
    require(
        okay,
        "Changed another category, temporary inventory or unrelated save data");
    Save reread(save.bytes());
    require(reread.bytes() == save.bytes(),
            "Inventory edit failed structural reparse");
  };
  auto undoAll = [&] {
    while (save.undo()) {
    }
    require(save.bytes() == original, "Inventory undo not byte-identical");
  };
  auto inGroup = [&](ItemGroup group) {
    std::vector<Item> result;
    for (auto item : model.items(save))
      if (item.group == group)
        result.push_back(item);
    return result;
  };
  for (int g = 0; g < 9; ++g) {
    auto group = ItemGroup(g);
    require(model.capacity(save, group) == capacities[g],
            "Wrong ordinary pouch capacity");
    auto all = inGroup(group);
    require(!all.empty(), "Missing test category");
    auto first = std::find_if(all.begin(), all.end(),
                              [&](auto &i) { return model.canManage(i); });
    if (group == ItemGroup::KeyItems) {
      auto before = save.bytes();
      refuses([&] { model.removeItem(save, all.front()); },
              "Key item removal was allowed");
      require(save.bytes() == before, "Rejected removal modified bytes");
      continue;
    }
    require(first != all.end(), "No manageable fixture item");
    auto item = *first;
    auto p = std::string("Pouch.") + groups[g] + ".Content.";
    auto h = totk::hash(p + "Name");
    auto oldCount = all.size();
    auto before = save.bytes();
    model.removeItem(save, item);
    require(inGroup(group).size() == oldCount - 1, "Removal failed");
    normalOnly(before, group);
    Save previous(before);
    for (size_t i = item.ordinal; i + 1 < capacities[g]; ++i)
      require(save.string(h, 64, i) == previous.string(h, 64, i + 1),
              "Removal failed to compact names");
    for (auto property :
         {"Life", "StockNum", "Effect.Type", "Effect.Value", "ColorVariation",
          "Combined.Name", "Combined.Life", "ExtraLife", "RecordExtraLife",
          "LifeRecover", "Effect.Level", "Effect.Time", "Price", "GetOrder",
          "UseOrder", "MaterialName"}) {
      auto k = totk::hash(p + property);
      if (!save.count(k))
        continue;
      size_t width = save.count(k) / (capacities[g] * 2);
      bool same = true;
      for (size_t i = item.ordinal; i + 1 < capacities[g]; ++i)
        for (size_t j = 0; j < width; ++j)
          same &= save.word(k, i * width + j) ==
                  previous.word(k, (i + 1) * width + j);
      require(same, "Removal detached item companion fields");
    }
    for (auto ref : {"EquipIndex", "EquipIndexes.Head", "EquipIndexes.Upper",
                     "EquipIndexes.Bottom"}) {
      auto k = totk::hash(std::string("Pouch.") + groups[g] + "." + ref);
      if (!save.count(k))
        continue;
      auto old = previous.integer(k);
      auto expected = old == int(item.ordinal)  ? -1
                      : old > int(item.ordinal) ? old - 1
                                                : old;
      require(save.integer(k) == expected,
              "Removal lost equipped-item reference");
    }
    auto removed = save.bytes();
    require(save.undo() && save.bytes() == before, "Single remove undo failed");
    require(save.redo() && save.bytes() == removed, "Remove redo failed");
    undoAll();
    // Create a free slot by removing the last ordinary item. Add must not use
    // unused capacity in the temporary challenge pouch when the main one is
    // full.
    auto last = all.back();
    model.removeItem(save, last);
    auto baseline = save.bytes();
    model.addItem(save, group, last.id);
    normalOnly(baseline, group);
    require(inGroup(group).size() == oldCount,
            "Addition did not restore item count");
    if (g == 0 || g == 1 || g == 3) {
      auto entries = inGroup(group);
      auto added = entries.back();
      require(save.integer(totk::hash(p + "Life"), added.ordinal) == 1,
              "New equipment durability default wrong");
    }
    undoAll();
    if (model.canDuplicate(group)) {
      if (!model.availableSlots(save, group))
        model.removeItem(save, last);
      auto count = inGroup(group).size();
      auto baseline = save.bytes();
      model.duplicateItem(save, item);
      require(inGroup(group).size() == count + 1, "Duplicate count wrong");
      normalOnly(baseline, group);
      auto entries = inGroup(group);
      size_t duplicate = 0;
      bool found = false;
      for (auto v : entries)
        if (v.id == item.id && v.ordinal != item.ordinal &&
            Save(baseline).string(h, 64, v.ordinal).empty()) {
          duplicate = v.ordinal;
          found = true;
          break;
        }
      require(found, "Duplicated slot missing");
      for (auto property :
           {"Life", "Effect.Type", "Effect.Value", "ColorVariation",
            "Combined.Name", "Combined.Life", "ExtraLife", "RecordExtraLife",
            "LifeRecover", "Effect.Level", "Effect.Time", "Price", "StockNum",
            "MaterialName"}) {
        auto k = totk::hash(p + property);
        if (!save.count(k))
          continue;
        size_t width = save.count(k) / (capacities[g] * 2);
        for (size_t j = 0; j < width; ++j)
          require(save.word(k, item.ordinal * width + j) ==
                      save.word(k, duplicate * width + j),
                  "Duplicate lost item companion data");
      }
      undoAll();
    }
    // Existing IDs are a conservative fallback when game names are unavailable.
    auto ids = model.catalog(save, group);
    require(std::find(ids.begin(), ids.end(), item.id) != ids.end(),
            "Existing item absent from catalogue");
    auto snapshot = save.bytes();
    refuses([&] { model.addItem(save, group, "Not_A_Game_Item"); },
            "Unknown item ID was accepted");
    require(save.bytes() == snapshot, "Rejected add changed data");
    auto stale = item;
    stale.id = "stale";
    refuses([&] { model.removeItem(save, stale); },
            "Stale item reference accepted");
    if (group == ItemGroup::Materials || group == ItemGroup::Arrows ||
        group == ItemGroup::Unknown) {
      auto before = save.bytes();
      model.fillStacks(save, group);
      normalOnly(before, group);
      for (auto owned : inGroup(group))
        require(save.integer(totk::hash(p + "StockNum"), owned.ordinal) == 999,
                "Stack not filled");
      undoAll();
    }
    if (group == ItemGroup::Weapons || group == ItemGroup::Bows ||
        group == ItemGroup::Shields || group == ItemGroup::Armor) {
      bool was = model.equipped(save, item);
      auto before = save.bytes();
      model.equipItem(save, item);
      require(model.equipped(save, item) != was, "Equip toggle failed");
      normalOnly(before, group);
      undoAll();
    }
  }
  // A full ordinary bow pouch must fail without using the empty second pouch.
  auto bows = inGroup(ItemGroup::Bows);
  while (model.availableSlots(save, ItemGroup::Bows))
    model.duplicateItem(save, bows.front());
  auto full = save.bytes();
  refuses([&] { model.duplicateItem(save, bows.front()); },
          "Full ordinary pouch accepted a duplicate");
  refuses([&] { model.addItem(save, ItemGroup::Bows, bows.front().id); },
          "Full ordinary pouch accepted an add");
  require(save.bytes() == full, "Full-pouch refusal changed bytes");
  undoAll();
  auto stats = model.stats(save);
  for (auto &f : stats)
    if (f.field.name == "Pouch.Bow.ValidNum")
      refuses([&] { setValue(save, f, "5"); },
              "Capacity reduction hid existing items");
  auto weapon = inGroup(ItemGroup::Weapons);
  auto first = std::find_if(weapon.begin(), weapon.end(),
                            [&](auto &i) { return model.canManage(i); });
  require(first != weapon.end(), "Replacement fixture missing");
  auto choices = model.catalog(save, ItemGroup::Weapons);
  auto replacement = std::find_if(choices.begin(), choices.end(),
                                  [&](auto &id) { return id != first->id; });
  require(replacement != choices.end(), "Replacement candidate missing");
  auto beforeReplace = save.bytes();
  model.replaceItem(save, *first, *replacement);
  auto key = totk::hash("Pouch.Weapon.Content.Name");
  std::set<size_t> nameBytes;
  for (size_t i = 0; i < 16; ++i) {
    auto at = save.fields().at(key)[first->ordinal * 16 + i];
    for (size_t b = 0; b < 4; ++b)
      nameBytes.insert(at + b);
  }
  bool onlyName = true;
  for (size_t i = 0; i < beforeReplace.size(); ++i)
    if (beforeReplace[i] != save.bytes()[i] && !nameBytes.count(i))
      onlyName = false;
  require(onlyName, "Replacing item identity changed other values");
  normalOnly(beforeReplace, ItemGroup::Weapons);
  undoAll();
  refuses([&] { model.replaceItem(save, *first, "NormalArrow"); },
          "Cross-category replacement accepted");
  auto materials = inGroup(ItemGroup::Materials);
  refuses(
      [&] { model.addItem(save, ItemGroup::Materials, materials.front().id); },
      "Duplicate material stack accepted");
  for (auto &item : weapon)
    if (!model.canManage(item)) {
      refuses([&] { model.duplicateItem(save, item); },
              "Protected weapon duplication accepted");
      break;
    }
  auto malformed = original;
  totk::Save parsed(malformed);
  auto life = parsed.find("Pouch.Bow.Content.Life");
  put(malformed, life->offset - 4, uint32_t(life->count - 1));
  Save malformedSave(malformed);
  refuses([&] { model.removeItem(malformedSave, bows.front()); },
          "Mismatched parallel array length accepted");
  require(malformedSave.bytes() == malformed, "Refused layout wrote data");
  for (auto &item : model.items(save))
    if (item.group == ItemGroup::Weapons &&
        model.itemDetail(save, item).find("Fused: ") != std::string::npos) {
      auto before = save.bytes();
      auto life =
          save.integer(totk::hash("Pouch.Weapon.Content.Life"), item.ordinal);
      model.clearFusion(save, item);
      require(model.itemDetail(save, item).find("Fused: ") == std::string::npos,
              "Fuse name not cleared");
      require(save.integer(totk::hash("Pouch.Weapon.Content.Life"),
                           item.ordinal) == life,
              "Clearing fusion changed base durability");
      normalOnly(before, item.group);
      undoAll();
      break;
    }
  // The private second pouch is intentionally not surfaced, even when
  // populated.
  auto bowNames = totk::hash("Pouch.Bow.Content.Name");
  save.edit("Temporary pouch test",
            [&] { save.string(bowNames, 64, 14, "temporary-only-bow"); });
  for (auto &item : model.items(save))
    require(item.id != "temporary-only-bow",
            "Temporary inventory leaked into ordinary inventory");
  undoAll();
}
