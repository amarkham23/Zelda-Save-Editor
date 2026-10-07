// Copyright (c) 2026 StockiestThree9. MIT.
// TOTK-specific menus share the main editor's rows, confirmations and history.
bool totkRows(View view, std::vector<Row> &r) {
  if (view == View::Sections) {
    r.push_back({"Player", "Hearts, stamina wheels, battery cells and rupees", "", [this] {
                   fieldList("Player", tears.stats(*save));
                   current().expanded = true;
                 }});
    r.push_back({"Inventory", "Add, replace, duplicate and customize items", "", [this] {
                   push(View::Inventory, "Inventory",
                        metadata.names.empty() ? "Item IDs shown - reload Game resources"
                                               : "L / R changes section");
                 }});
    r.push_back({"World & completion", "Collectibles, quests, compendium and map stamps", "",
                 [this] { push(View::TotkWorld, "World & completion"); }});
    r.push_back(
        {"Horses", "Names, bond, stats and tack", "", [this] { push(View::Horses, "Horses"); }});
    r.push_back({"Autobuild", "Favorites and blueprint import / export", "",
                 [this] { push(View::Autobuild, "Autobuild"); }});
    r.push_back({"Advanced", "Named fields and unmapped save values", "", [this] {
                   fieldList("Advanced", tears.matching(*save, [](auto &) { return true; }),
                             "Direct fields - # labels have no verified name");
                 }});
  } else if (view == View::Abilities) {
    for (auto f : tears.abilities(*save)) {
      auto id = f.field.name.substr(10);
      bool on = save->boolean(f.field.hash);
      r.push_back({f.label, "Story quests stay as saved", on ? "Available" : "Unavailable",
                   [this, id, on, f] {
                     confirm(std::string(on ? "Disable " : "Enable ") + f.label + "?",
                             "Changes ability availability. Associated quests and story rewards "
                             "remain separate.",
                             "Apply", [this, id, on] { tears.setAbility(*save, id, !on); });
                   }});
    }
  } else if (view == View::TotkWorld) {
    for (auto category : {"Koroks", "Shrines", "Lightroots", "Towers", "Bubbulfrogs", "Caves",
                          "Wells", "Chasms", "Locations"}) {
      auto fields = tears.worldFields(*save, category);
      size_t done = 0;
      for (auto &f : fields)
        done += worldDone(f);
      r.push_back({category, "Discovery and completion saved in this slot",
                   std::to_string(done) + " / " + std::to_string(fields.size()), [this, category] {
                     push(View::WorldCategory, category);
                     current().worldCategory = category;
                   }});
    }
    r.push_back({"Compendium", "Named records and stock pictures", "", [this] {
                   fieldList("Compendium", tears.matching(*save, [](auto &f) {
                     return f.name.rfind("PictureBookData.", 0) == 0 && f.name.size() > 6 &&
                            f.name.compare(f.name.size() - 6, 6, ".State") == 0;
                   }));
                 }});
    r.push_back({"Quests", "Choose a quest, then its saved stage", "", [this] {
                   push(View::Quests, "Quests", "Direct stages do not reconcile other story flags");
                 }});
    r.push_back({"Map stamps", "Icon, layer and coordinates", "",
                 [this] { push(View::StampList, "Map stamps"); }});
    r.push_back({"Amiibo", "Clear today's scans while keeping lifetime counts", "Reset", [this] {
                   confirm("Reset amiibo scans?",
                           "Clears the saved touch dates. Character IDs and lifetime scan counts "
                           "stay as saved.",
                           "Reset", [this] {
                             tears.resetAmiibo(*save);
                             notice("Daily scans cleared");
                           });
                 }});
    r.push_back({"Saved position", "Coordinates in the currently saved map", "", [this] {
                   fieldList("Saved position", tears.matching(*save, [](auto &f) {
                     return f.name == "PlayerStatus.SavePos";
                   }));
                 }});
    r.push_back({"Enemy defeat counts", "Saved kills used by enemy scaling", "", [this] {
                   fieldList("Enemy defeat counts", tears.matching(*save, [](auto &f) {
                     return f.name.rfind("DefeatedEnemyNum.", 0) == 0;
                   }));
                 }});
    r.push_back({"Pristine weapon spawns", "Register decayed weapons as previously broken",
                 "Unlock", [this] {
                   confirm("Unlock pristine weapon spawns?",
                           "Sets verified broken-weapon counts to at least one. Existing world "
                           "drops are retained.",
                           "Unlock", [this] {
                             tears.unlockPristine(*save);
                             notice("Pristine spawn requirements updated");
                           });
                 }});
  } else if (view == View::WorldCategory) {
    auto category = current().worldCategory;
    auto fields = tears.worldFields(*save, category);
    if (!fields.empty())
      r.push_back(
          {"Mark all as complete", "Updates this category's flags; quest rewards are separate",
           "Apply", [this, category] {
             confirm("Complete all " + category + "?",
                     "Changes discovery or completion flags in this save. Quest rewards and "
                     "related story progress are not granted. Undo is available before saving.",
                     "Apply", [this, category] {
                       tears.completeWorld(*save, category);
                       notice("Completion flags updated");
                     });
           }});
    for (auto f : fields)
      if (matches(f.label + " " + f.description + " " + f.field.name))
        r.push_back({f.label, f.description,
                     f.field.kind == Kind::Bool ? (worldDone(f) ? "Found" : "Not found")
                                                : displayValue(*save, f),
                     [this, f] { editField(f); }});
    if (fields.empty())
      r.push_back({"Game resources needed",
                   "Settings > Game resources loads the installed world tables", "", [this] {
                     message("World records",
                             "Reload Game resources while launching Sphaira over TOTK. No save "
                             "import or separate check app is required.");
                   }});
  } else if (view == View::Quests) {
    for (auto &pair : metadata.quests) {
      auto fields = tears.questFields(*save, pair.first);
      if (fields.empty())
        continue;
      auto it = metadata.questNames.find(pair.first);
      auto name = it == metadata.questNames.end() ? readable(pair.first) : it->second;
      if (!matches(name + " " + pair.first))
        continue;
      auto id = pair.first;
      r.push_back(
          {name, "Saved quest stage", displayValue(*save, fields.front()), [this, id, name] {
             fieldList(name, tears.questFields(*save, id),
                       "Direct stages - related story flags are separate");
             current().expanded = true;
           }});
    }
  } else if (view == View::StampList) {
    auto h = totk::hash("MapData.IconData.StampData.Type");
    r.push_back({"Clear all stamps", "Removes map icons; colored pins remain", "", [this] {
                   confirm("Clear map stamps?",
                           "All stamp icons will be removed. This can be undone before saving.",
                           "Clear stamps", [this] {
                             tears.clearMapStamps(*save);
                             notice("Map stamps cleared");
                           });
                 }});
    for (size_t i = 0; i < save->logicalCount(h); ++i) {
      auto fields = tears.mapFields(*save, i);
      if (fields.empty())
        continue;
      auto label = "Stamp " + std::to_string(i + 1);
      auto shown = displayValue(*save, fields.front());
      if (!matches(label + " " + shown))
        continue;
      r.push_back({label, shown, save->word(h, i) == totk::hash("Invalid") ? "Empty" : "",
                   [this, i, label] {
                     fieldList(label, tears.mapFields(*save, i));
                     current().expanded = true;
                   }});
    }
  } else if (view == View::Horses) {
    auto h = totk::hash("OwnedHorseList.ActorName");
    for (size_t i = 0; i < save->logicalCount(h); ++i) {
      auto actor = save->string(h, 64, i);
      if (actor.empty())
        continue;
      auto name = save->wideString(totk::hash("OwnedHorseList.Name"), i);
      if (name.empty())
        name = "Horse " + std::to_string(i + 1);
      r.push_back({name, "Registered horse " + std::to_string(i + 1), "", [this, i, name] {
                     fieldList(name, tears.horseFields(*save, i));
                     current().expanded = true;
                   }});
    }
  } else if (view == View::Autobuild) {
    auto h = totk::hash("AutoBuilder.Draft.Content.Index");
    for (size_t i = 0; i < save->logicalCount(h); ++i) {
      auto b = tears.blueprint(*save, i);
      auto label = "Blueprint " + std::to_string(i + 1);
      r.push_back({label, b.index < 0 ? "Empty slot" : "Saved build", b.favorite ? "Favorite" : "",
                   [this, i, label] {
                     push(View::Blueprint, label,
                          "Import / export uses Zelda Save Editor .zsbp files");
                     current().record = i;
                   }});
    }
  } else if (view == View::Blueprint) {
    auto index = current().record;
    auto b = tears.blueprint(*save, index);
    r.push_back({"Favorite", "Keep this build in your favorites", b.favorite ? "Yes" : "No",
                 [this, index] { tears.favoriteBlueprint(*save, index); }});
    r.push_back({"Export blueprint", "Saves a .zsbp file in the editor's blueprints folder",
                 "Export", [this, index] {
                   auto folder = platform.dataPath() + "/blueprints";
                   std::filesystem::create_directories(folder);
                   auto file = folder + "/blueprint-" + std::to_string(index + 1) + "-" +
                               std::to_string(std::time(nullptr)) + ".zsbp";
                   auto bytes = tears.exportBlueprint(*save, index);
                   writeFile(file, bytes);
                   if (readFile(file) != bytes)
                     throw Error("Blueprint export verification failed.");
                   notice("Blueprint exported to SD");
                 }});
    r.push_back({"Import blueprint", "Choose a .zsbp from the editor's blueprints folder", "Choose",
                 [this, index] {
                   push(View::BlueprintImports, "Import blueprint",
                        "Select a Zelda Save Editor .zsbp file");
                   current().record = index;
                 }});
  } else if (view == View::BlueprintImports) {
    auto index = current().record;
    auto folder = platform.dataPath() + "/blueprints";
    std::filesystem::create_directories(folder);
    std::vector<std::filesystem::path> files;
    for (auto &file : std::filesystem::directory_iterator(folder))
      if (file.is_regular_file() && file.path().extension() == ".zsbp")
        files.push_back(file.path());
    std::sort(files.begin(), files.end());
    for (auto file : files)
      r.push_back(
          {file.filename().string(), "Replace this slot's build; other blueprints stay as saved",
           "", [this, index, file] {
             confirm("Import this blueprint?",
                     file.filename().string() +
                         " will replace the selected build. Undo is available before saving.",
                     "Import", [this, index, file] {
                       tears.importBlueprint(*save, index, readFile(file.string(), 128 * 1024));
                       back();
                       notice("Blueprint imported");
                     });
           }});
  } else if (view == View::FusionPicker) {
    auto item = current().item;
    for (auto id : tears.fusionCatalog(*save)) {
      auto name = metadata.itemName(id);
      if (!matches(name + " " + id))
        continue;
      r.push_back({name, "Keeps base durability and fuse bonus", "", [this, item, id] {
                     tears.setFusion(*save, item, id);
                     back();
                     current().subtitle = tears.itemDetail(*save, item);
                     notice("Attachment updated");
                   }});
    }
  } else
    return false;
  return true;
}
