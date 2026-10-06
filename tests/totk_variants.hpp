// Copyright (c) 2026 StockiestThree9. MIT.
// Mapping and byte-preservation regressions. Installed resources stay private.
static void testVariantLinks() {
  Metadata data;
  auto armor = [&](const std::string &id, int stars, const std::string &next) {
    auto &v = data.variants[id];
    v.armor = true;
    v.category = "Armor";
    v.stars = stars;
    v.nextArmor = next;
  };
  armor("Armor_Base_Head", 0, "Armor_Upgrade_Head");
  armor("Armor_Upgrade_Head", 1, "");
  armor("Armor_Internal_Head", 0, "Armor_Upgrade_Head");
  data.names["Armor_Base_Head"] = data.names["Armor_Upgrade_Head"] =
      "Example cap";
  linkTotkVariants(data);
  require(data.variants.at("Armor_Upgrade_Head").family == "Armor_Base_Head",
          "Unnamed actor split a named armor upgrade chain");
  data.names["Armor_Internal_Head"] = "Example cap";
  linkTotkVariants(data);
  require(data.variants.at("Armor_Upgrade_Head").family == "Armor_Upgrade_Head",
          "Ambiguous armor chain was guessed");
  data.variants.at("Armor_Base_Head").nextArmor = "Armor_Base_Head";
  data.variants.at("Armor_Internal_Head").nextArmor = "";
  linkTotkVariants(data);
  require(data.variants.at("Armor_Base_Head").family == "Armor_Base_Head",
          "Cyclic chain was accepted");
  auto &a = data.variants["Weapon_Sword_A"];
  a.category = "Weapon";
  a.model = "Example_Before";
  a.modelProject = "Example";
  auto &b = data.variants["Weapon_Sword_B"];
  b.category = "Weapon";
  b.model = "Example";
  b.modelProject = "Example";
  linkTotkVariants(data);
  require(a.condition == "Pristine" && b.condition == "Decayed" &&
              a.family == b.family,
          "Explicit weapon model pair not linked");
  data.variants["Weapon_Sword_C"] = b;
  linkTotkVariants(data);
  require(a.condition.empty() && b.condition.empty(),
          "Ambiguous weapon model pair was guessed");
  // Version 7 wide BYML values and bounds, both byte orders.
  for (bool big : {false, true}) {
    Bytes bytes(36);
    bytes[0] = big ? 'B' : 'Y';
    bytes[1] = big ? 'Y' : 'B';
    auto number = [&](size_t at, uint32_t value, size_t size) {
      for (size_t i = 0; i < size; ++i)
        bytes.at(at + i) = uint8_t(value >> ((big ? size - i - 1 : i) * 8));
    };
    number(2, 7, 2);
    number(12, 16, 4);
    bytes[16] = 0xc0;
    number(17, 1, 3);
    bytes[20] = 0xd5;
    number(24, 28, 4);
    number(28, big ? 0x12345678 : 0x9abcdef0, 4);
    number(32, big ? 0x9abcdef0 : 0x12345678, 4);
    require(readByml(bytes).array.front().wide == 0x123456789abcdef0ULL,
            "Wide BYML integer or byte order incorrect");
    bytes.pop_back();
    refuses([&] { readByml(bytes); }, "Truncated wide BYML value accepted");
  }
}
static void testInstalledVariants(const Bytes &original,
                                  const std::string &path) {
  Metadata data;
  loadTotkNames(data, path, [](const auto &s) { std::cout << s << '\n'; });
  require(!data.variants.empty(),
          "Private installed catalogue fixture missing");
  Totk model(data);
  Save save(original);
  model.definitions(save);
  auto armor = model.families(save, ItemGroup::Armor);
  auto weapons = model.families(save, ItemGroup::Weapons);
  require(armor.size() == 135 && weapons.size() == 80,
          "Installed catalogue grouping changed");
  auto hood = std::find_if(armor.begin(), armor.end(),
                           [](auto &f) { return f.id == "Armor_001_Head"; });
  require(hood != armor.end() && hood->ids.size() == 10,
          "Hylian Hood was not grouped into one entry");
  const char *up[] = {"Armor_001_Head", "Armor_002_Head", "Armor_003_Head",
                      "Armor_004_Head", "Armor_015_Head"};
  const char *down[] = {"Armor_1152_Head", "Armor_1153_Head", "Armor_1154_Head",
                        "Armor_1155_Head", "Armor_1156_Head"};
  for (int rank = 0; rank < 5; ++rank)
    for (bool lowered : {false, true}) {
      std::string id = lowered ? down[rank] : up[rank];
      const auto &v = data.variants.at(id);
      require(v.stars == rank && v.style == (lowered ? "Hood down" : "Hood up"),
              "Installed hood rank/style incorrect");
      auto ranks = model.variantOptions(save, ItemGroup::Armor, id,
                                        VariantProperty::Stars);
      require(ranks.size() == 5, "Hood upgrade choices missing");
      for (int i = 0; i < 5; ++i)
        require(ranks[i].first == (lowered ? down[i] : up[i]),
                "Changing stars changed hood position");
      auto styles = model.variantOptions(save, ItemGroup::Armor, id,
                                         VariantProperty::Style);
      require(styles.size() == 2, "Hood position choices missing");
      for (auto &choice : styles)
        require(data.variants.at(choice.first).stars == rank,
                "Changing hood position changed stars");
    }
  require(model.initialDurability("Weapon_Sword_001") == 20 &&
              model.initialDurability("Weapon_Sword_106") == 16,
          "Traveler's Sword base durability incorrect");
  require(model.variantLabel("Weapon_Sword_001") == "Pristine" &&
              model.variantLabel("Weapon_Sword_106") == "Decayed",
          "Weapon condition is unclear or reversed");
  auto conditions = model.variantOptions(
      save, ItemGroup::Weapons, "Weapon_Sword_001", VariantProperty::Condition);
  require(conditions.size() == 2, "Weapon condition choices missing");
  require(model
              .variantOptions(save, ItemGroup::Weapons, "Weapon_Sword_103",
                              VariantProperty::Condition)
              .empty(),
          "Wooden Stick was labeled as decayed/pristine");
  require(model.variantOptions(save, ItemGroup::Weapons, "Weapon_Sword_103",
                               VariantProperty::Style)
                  .size() == 2,
          "Stick appearance variants are not grouped");
  for (auto group : {ItemGroup::Armor, ItemGroup::Weapons}) {
    Item item;
    bool found = false;
    for (auto candidate : model.items(save))
      if (candidate.group == group && model.canManage(candidate) &&
          (group != ItemGroup::Armor ||
           candidate.id.substr(candidate.id.rfind('_') + 1) == "Head")) {
        item = candidate;
        found = true;
        break;
      }
    require(found, "Variant test inventory item missing");
    model.replaceItem(save, item,
                      group == ItemGroup::Armor ? up[3] : "Weapon_Sword_001");
    item.id = group == ItemGroup::Armor ? up[3] : "Weapon_Sword_001";
    auto baseline = save.bytes();
    auto replacement = group == ItemGroup::Armor ? down[3] : "Weapon_Sword_106";
    model.changeVariant(save, item, replacement);
    auto key =
        totk::hash(group == ItemGroup::Armor ? "Pouch.Armor.Content.Name"
                                             : "Pouch.Weapon.Content.Name");
    std::set<size_t> allowed;
    for (size_t i = 0; i < 16; ++i) {
      auto at = save.fields().at(key)[item.ordinal * 16 + i];
      for (size_t b = 0; b < 4; ++b)
        allowed.insert(at + b);
    }
    bool onlyName = true;
    auto changed = save.bytes();
    for (size_t i = 0; i < baseline.size(); ++i)
      if (baseline[i] != changed[i] && !allowed.count(i))
        onlyName = false;
    require(onlyName && baseline != changed,
            "Variant change touched durability, dye, equipment, fusion or "
            "other data");
    require(save.undo() && save.bytes() == baseline, "Variant undo differs");
    require(save.redo() && save.bytes() == changed, "Variant redo differs");
    item.id = replacement;
    refuses(
        [&] {
          model.changeVariant(save, item,
                              group == ItemGroup::Armor ? "Armor_001_Upper"
                                                        : "Weapon_Sword_103");
        },
        "Cross-family variant accepted");
    require(save.bytes() == changed, "Rejected variant edited data");
    while (save.undo()) {
    }
    require(save.bytes() == original, "Variant test undo failed");
  }
  for (const char *id : {"Weapon_Sword_001", "Weapon_Sword_106"}) {
    if (!model.availableSlots(save, ItemGroup::Weapons))
      for (auto item : model.items(save))
        if (item.group == ItemGroup::Weapons && model.canManage(item)) {
          model.removeItem(save, item);
          break;
        }
    auto added = model.addItem(save, ItemGroup::Weapons, id);
    require(save.integer(totk::hash("Pouch.Weapon.Content.Life"),
                         added.ordinal) == model.initialDurability(id),
            "Added weapon did not receive base durability");
    while (save.undo()) {
    }
    require(save.bytes() == original, "New variant undo failed");
  }
  std::cout << "Installed catalogue: 552 armor variants -> 135 choices; 115 "
               "weapons -> 80 choices\n";
}
