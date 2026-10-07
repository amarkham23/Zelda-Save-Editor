// Copyright (c) 2026 StockiestThree9. MIT.
static void testFeatures(const Bytes &original, const std::string &resources) {
  Metadata metadata;
  loadTotkNames(metadata, resources,
                [](auto &message) { std::cout << "Resource: " << message << '\n'; });
  Save save(original);
  Totk model(metadata);
  model.definitions(save);
  require(metadata.enums.size() > 1000, "Installed enum choices were not decoded");
  require(!metadata.quests.empty() && !metadata.attachments.empty() && metadata.world.size() > 900,
          "Feature resources were not decoded");
  std::cout << "Feature resources: " << metadata.enums.size() << " enums, "
            << metadata.quests.size() << " quests, " << metadata.attachments.size()
            << " attachments, " << metadata.world.size() << " world records\n";
  auto reset = [&] {
    while (save.undo()) {
    }
    require(save.bytes() == original && !save.dirty(), "Feature undo did not restore every byte");
  };
  auto changesOnly = [&](uint32_t h, size_t start, size_t count) {
    std::set<size_t> allowed;
    for (size_t i = start; i < start + count; ++i) {
      auto offset = save.fields().at(h).at(i);
      for (size_t b = 0; b < 4; ++b)
        allowed.insert(offset + b);
    }
    for (size_t p = 0; p < original.size(); ++p)
      if (save.bytes()[p] != original[p])
        require(allowed.count(p), "Feature changed an unrelated byte");
  };
  auto name = totk::hash("OwnedHorseList.Name");
  require(save.logicalCount(name) == 10, "Horse name count is wrong");
  save.edit("Unicode horse name", [&] { save.wideString(name, 0, u8"Epona \u96ea \U0001F434"); });
  require(save.wideString(name, 0) == u8"Epona \u96ea \U0001F434",
          "UTF-16 horse name did not round trip");
  changesOnly(name, 0, 8);
  auto changed = save.bytes();
  require(save.undo() && save.bytes() == original, "Horse name undo mismatch");
  require(save.redo() && save.bytes() == changed, "Horse name redo mismatch");
  reset();
  refuses([&] { save.edit("Too long", [&] { save.wideString(name, 0, std::string(16, 'x')); }); },
          "Unterminated long name was accepted");
  require(save.bytes() == original, "Rejected horse name changed bytes");
  refuses(
      [&] {
        save.edit("Invalid text", [&] { save.wideString(name, 0, std::string("\xc0\x80", 2)); });
      },
      "Invalid UTF-8 was accepted");
  auto favorite = totk::hash("AutoBuilder.Draft.Content.IsFavorite");
  std::vector<bool> before;
  for (size_t i = 0; i < save.logicalCount(favorite); ++i)
    before.push_back(save.boolean(favorite, i));
  save.edit("Packed bit", [&] { save.boolean(favorite, 2, !before[2]); });
  for (size_t i = 0; i < before.size(); ++i)
    require(save.boolean(favorite, i) == (i == 2 ? !before[i] : before[i]),
            "Editing one packed boolean changed another");
  changesOnly(favorite, 0, 1);
  reset();
  auto blueprint = model.exportBlueprint(save, 0);
  model.importBlueprint(save, 0, blueprint);
  require(save.bytes() == original, "Same-slot blueprint import was not byte-identical");
  auto broken = blueprint;
  broken[50] ^= 1;
  refuses([&] { model.importBlueprint(save, 0, broken); },
          "Corrupt blueprint checksum was accepted");
  require(save.bytes() == original, "Refused blueprint modified save");
  auto source = model.blueprint(save, 1);
  model.importBlueprint(save, 0, model.exportBlueprint(save, 1));
  require(model.blueprint(save, 0).data == source.data, "Blueprint payload was not imported");
  require(model.blueprint(save, 1).data == source.data, "Source blueprint changed");
  reset();
  auto horses = model.horseFields(save, 0);
  require(horses.size() >= 6, "Horse editing fields are missing");
  for (auto &f : horses)
    std::cout << "Horse " << f.label << ": " << displayValue(save, f) << '\n';
  for (auto group : {ItemGroup::Weapons, ItemGroup::Bows, ItemGroup::Shields}) {
    model.repairEquipment(save, group);
    totk::Save reread(save.bytes());
    reset();
  }
  model.upgradeArmor(save);
  for (auto &item : model.items(save))
    if (item.group == ItemGroup::Armor)
      for (auto &option : model.variantOptions(save, item.group, item.id, VariantProperty::Stars))
        require(metadata.variants.at(item.id).stars >= metadata.variants.at(option.first).stars,
                "Armor did not reach highest rank");
  reset();
  model.restorePristine(save);
  for (auto &item : model.items(save))
    if (item.group == ItemGroup::Weapons && model.canManage(item)) {
      auto v = metadata.variants.find(item.id);
      if (v != metadata.variants.end())
        require(
            v->second.condition != "Decayed" ||
                model.variantOptions(save, item.group, item.id, VariantProperty::Condition).size() <
                    2,
            "Decayed weapon remained");
    }
  reset();
  auto attachments = model.fusionCatalog(save);
  require(attachments.size() > 100, "Attachment catalogue is missing");
  auto all = model.items(save);
  auto weapon = std::find_if(all.begin(), all.end(), [](auto &item) {
    return item.group == ItemGroup::Weapons && item.id != "Weapon_Sword_070";
  });
  require(weapon != all.end(), "No fusion fixture");
  auto base = save.integer(totk::hash("Pouch.Weapon.Content.Life"), weapon->ordinal);
  model.setFusion(save, *weapon, attachments.front());
  require(save.string(totk::hash("Pouch.Weapon.Content.Combined.Name"), 64, weapon->ordinal) ==
              attachments.front(),
          "Attachment identity was not set");
  require(save.integer(totk::hash("Pouch.Weapon.Content.Life"), weapon->ordinal) == base,
          "Fusion changed base durability");
  reset();
  for (auto category : {"Koroks", "Shrines", "Lightroots", "Towers", "Bubbulfrogs", "Caves",
                        "Wells", "Chasms", "Locations"}) {
    auto fields = model.worldFields(save, category);
    std::cout << category << " editable records: " << fields.size() << '\n';
    if (fields.empty())
      continue;
    model.completeWorld(save, category);
    for (auto &f : fields) {
      auto value = save.word(f.field.hash);
      require(value == 1 || value == totk::hash("Clear") || value == totk::hash("Open"),
              "Bulk completion left an unsupported state");
    }
    reset();
  }
  auto compendium = model.matching(save, [](auto &f) {
    return f.name.rfind("PictureBookData.", 0) == 0 && f.name.find(".State") != std::string::npos;
  });
  std::cout << "Compendium: " << compendium.size() << '\n';
  require(compendium.size() > 300, "Compendium fields are missing");
  model.unlockCompendium(save);
  for (auto &f : compendium)
    require(save.word(f.field.hash) != totk::hash("Unopened"), "Compendium record stayed locked");
  reset();
  size_t quests = 0;
  for (auto &q : metadata.quests)
    if (!model.questFields(save, q.first).empty())
      ++quests;
  std::cout << "Editable quests: " << quests << '\n';
  require(quests > 100, "Quest stage fields are missing");
  auto stamps = model.mapFields(save, 0);
  require(stamps.size() == 4, "Map stamp fields are incomplete");
  model.clearMapStamps(save);
  auto h = totk::hash("MapData.IconData.StampData.Type");
  for (size_t i = 0; i < save.count(h); ++i)
    require(save.word(h, i) == totk::hash("Invalid"), "Stamp was not cleared");
  changesOnly(h, 0, save.count(h));
  reset();
  model.resetAmiibo(save);
  auto dates = totk::hash("AmiiboData.TouchLog.Year");
  for (size_t i = 0; i < save.count(dates); ++i)
    require(save.integer(dates, i) == 0, "Amiibo scans were not reset");
  reset();
  auto abilities = model.abilities(save);
  require(abilities.size() == 6, "Ability selectors are missing");
  model.setAbility(save, "UltraHand", false);
  require(!save.boolean(totk::hash("IsGet.Obj_UltraHand")) &&
              !save.boolean(totk::hash("IsGetAnyway.Obj_UltraHand")),
          "Ability ownership flags disagree");
  reset();
  model.unlockPristine(save);
  reset();
}
