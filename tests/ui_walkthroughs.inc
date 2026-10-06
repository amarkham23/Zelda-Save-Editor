// Copyright (c) 2026 StockiestThree9. MIT.
// Host-only in-memory UI checks, included inside App::run.
        if(argc>1&&std::string(argv[1])=="--totk-self-test"){
            auto ensure=[](bool ok,const char* why){if(!ok)throw Error(why);};
            std::filesystem::create_directories("preview");platform.settings.theme=Theme::Dark;
            current().selected=1;screenshot("preview/totk-games.png");handle(Input{A},rows());
            ensure(isTotk()&&current().view==View::Slots,"TOTK game selection failed");ensure(tabCount()==1,"TOTK exposed BOTW tabs");handle(Input{Y},rows());ensure(current().view==View::Slots,"TOTK exposed profile swapping");
            ensure(!slots.empty(),"TOTK fixture is missing");screenshot("preview/totk-saves.png");handle(Input{A},rows());ensure(save&&save->game()==Game::Totk,"Wrong game save opened");auto before=save->bytes();
            screenshot("preview/totk-sections.png");handle(Input{A},rows());ensure(current().view==View::Fields,"Player page missing");screenshot("preview/totk-player.png");
            for(int i=0;i<30;++i){handle(Input{Down},rows());handle(Input{Up},rows());}back();current().selected=1;handle(Input{A},rows());screenshot("preview/totk-inventory.png");
            for(int i=0;i<9;++i){bool add=false;for(auto& row:rows())add|=row.title=="Add item";ensure(add==(current().group!=ItemGroup::KeyItems),"Wrong TOTK item management availability");handle(Input{R},rows());}
            handle(Input{A},rows());ensure(current().view==View::Item,"Item page missing");screenshot("preview/totk-detail.png");auto detail=rows();for(auto& row:detail)if(row.title=="Equipment bonus"){row.action();break;}
            ensure(current().view==View::Choices,"Bonus picker missing");screenshot("preview/totk-bonus.png");for(int i=0;i<10;++i)handle(Input{Down},rows());back();ensure(save->bytes()==before,"TOTK navigation edited data");
            back();ensure(current().view==View::Inventory,"Inventory return failed");
            // Exercise real UI callbacks using this in-memory fixture only.
            Item lastWeapon;for(auto item:items())if(item.group==ItemGroup::Weapons)lastWeapon=item;
            tears.removeItem(*save,lastWeapon);auto freeSlot=save->bytes();
            for(auto row:rows())if(row.title=="Add item"){row.action();break;}
            ensure(current().view==View::Picker&&!rows().empty(),"TOTK catalogue missing");screenshot("preview/totk-catalogue.png");
            auto add=rows().front().action;add();ensure(current().view==View::ConfigureItem,"Item options missing");ensure(save->bytes()==freeSlot,"Choosing an item changed the save before confirmation");screenshot("preview/totk-item-options.png");auto apply=rows().back().action;apply();ensure(current().view==View::Item,"New item does not open for customization");screenshot("preview/totk-new-item.png");
            handle(Input{ZL},rows());ensure(current().view==View::Inventory&&save->bytes()==freeSlot,"Undo retained stale item callbacks");
            handle(Input{ZL},rows());ensure(save->bytes()==before,"UI undo failed to restore fixture");
            current().group=ItemGroup::Food;current().selected=0;handle(Input{A},rows());screenshot("preview/totk-meal.png");
            for(auto row:rows())if(row.title=="Meal effect"){row.action();break;}
            ensure(current().view==View::Choices,"Meal effect picker missing");screenshot("preview/totk-meal-effects.png");back();back();
            current().group=ItemGroup::Materials;current().selected=int(rows().size())-1;current().scroll=std::max(0,current().selected-5);screenshot("preview/totk-fill-stacks.png");
            ensure(save->bytes()==before,"Browsing new screens edited data");
            // Exercise pending options against private installed tables. No
            // live save write or commit is called by the UI walkthrough.
            if(!metadata.variants.empty()){
                auto choose=[&](const std::string& title){for(auto row:rows())if(row.title==title){ensure(bool(row.action),"Test row is not selectable");row.action();return;}throw Error("UI test row missing: "+title);};
                current().group=ItemGroup::Armor;current().selected=0;current().scroll=0;
                choose("Add item");ensure(current().view==View::Picker,"Armor catalogue missing");
                unsigned hoods=0;for(auto row:rows())hoods+=row.title=="Hylian Hood";ensure(hoods==1,"Repeated Hylian Hood choices remain");
                handle(Input{R},rows());ensure(current().catalogFilter==1,"Head filter missing");
                auto heads=rows();ensure(heads.size()<100,"Head filter did not shorten the list");
                for(auto row:heads){row.action();ensure(current().candidate.size()>5&&current().candidate.substr(current().candidate.size()-5)=="_Head","Non-head item in Head filter");back();}
                toast.clear();current().filter="hylian";screenshot("preview/totk-armor-catalogue.png");choose("Hylian Hood");
                choose("Upgrade level");choose("4 stars");ensure(current().candidate=="Armor_015_Head","Wrong four-star hood");
                choose("Hood position");choose("Hood down");ensure(current().candidate=="Armor_1156_Head","Changing hood position lost stars");
                ensure(save->bytes()==before,"Pending item options changed save");screenshot("preview/totk-hood-options.png");
                back();back();ensure(save->bytes()==before,"Cancelling item creation changed save");
                choose("Add item");choose("Hylian Hood");choose("Upgrade level");choose("4 stars");choose("Hood position");choose("Hood down");
                choose("Add to inventory");ensure(current().view==View::Item&&current().item.id=="Armor_1156_Head","Confirmed hood options were ignored");
                choose("Hood position");choose("Hood up");ensure(current().item.id=="Armor_015_Head","Owned hood style changed stars");
                choose("Upgrade level");choose("2 stars");ensure(current().item.id=="Armor_003_Head","Owned hood upgrades changed style");
                platform.settings.theme=Theme::Oled;toast.clear();screenshot("preview/totk-hood-detail.png");platform.settings.theme=Theme::Dark;
                handle(Input{ZL},rows());handle(Input{ZL},rows());handle(Input{ZL},rows());ensure(save->bytes()==before,"Hood UI edits failed exact undo");
                current().group=ItemGroup::Weapons;choose("Add item");handle(Input{R},rows());ensure(current().catalogFilter==1,"One-handed filter missing");
                toast.clear();current().filter="traveler";screenshot("preview/totk-weapon-catalogue.png");ensure(rows().size()==1,"Traveler's Sword duplicated or filter leaked weapon types");choose("Traveler's Sword");
                choose("Condition");choose("Decayed");ensure(current().candidate=="Weapon_Sword_106","Wrong decayed sword");
                bool life=false;for(auto row:rows())if(row.title=="Starting durability")life=row.value=="16";ensure(life,"Wrong durability shown for decayed sword");
                screenshot("preview/totk-weapon-options.png");choose("Condition");choose("Pristine");ensure(current().candidate=="Weapon_Sword_001","Wrong pristine sword");
                back();back();ensure(save->bytes()==before,"Weapon option browsing edited data");
            }
            // Restore the item-level depth expected by the shared Back-path check.
            current().group=ItemGroup::Weapons;current().selected=0;current().scroll=0;handle(Input{A},rows());
            back();back();back();back();ensure(current().view==View::Games,"TOTK back path incorrect");selectGame(Game::Botw);ensure(!isTotk()&&tabCount()>=3,"BOTW selection was contaminated by TOTK");
            std::cout<<"PASS: TOTK selection, isolated tabs, player and inventory browsing, enum picker, read-only navigation, return to BOTW; preview images saved\n";return 0;
        }
        if(argc>1&&std::string(argv[1])=="--self-test"){
            auto ensure=[](bool ok,const char* problem){if(!ok)throw Error(problem);};
            handle(Input{X},rows());ensure(current().view==View::Settings,"Settings unavailable before save");back();
            Input blank;blank.touchX=600;blank.touchY=500;handle(blank,rows());ensure(current().view==View::Games,"Empty game-picker space opened a game");
            chooseUser(platform.profiles());ensure(current().view==View::Slots&&stack.size()==2,"Single user was not skipped");back();ensure(current().view==View::Games,"Back returned to a skipped user screen");
            chooseUser(platform.profiles());auto originalSlots=slots;ensure(!slots.empty(),"No filter fixture");auto extra=slots.front();extra.gameProfile=2;extra.folder="ch_01/0";slots.push_back(extra);extra.gameProfile=1;extra.master=true;extra.folder="6";slots.push_back(extra);
            ensure(current().saveTab==SaveTab::Normal&&filteredSlots().size()==originalSlots.size(),"Normal was not the default filter");handle(Input{R},rows());ensure(current().saveTab==SaveTab::Profile2&&filteredSlots().size()==1&&filteredSlots()[0]->folder=="ch_01/0","Profile 2 filter mapped to the wrong save");
            handle(Input{R},rows());ensure(current().saveTab==SaveTab::Master&&filteredSlots().size()==1&&filteredSlots()[0]->folder=="6","Master Mode filter is incorrect");handle(Input{Y},rows());ensure(current().view==View::Slots,"Master Mode exposed normal-profile swapping");handle(Input{R},rows());ensure(current().saveTab==SaveTab::Normal&&current().selected==0&&current().scroll==0,"Tab cycling did not reset selection");
            slots=std::move(originalSlots);back();
            auto two=platform.profiles();two.push_back({3,4,"Second user",{}});chooseUser(two);ensure(current().view==View::Profiles,"Multiple users were not offered a choice");back();
            bool failure=false;try{busy("Test","Failure handling",[]{throw Error("Expected worker failure");});}catch(const Error&){failure=true;}ensure(failure,"Loading lost a worker exception");
            profiles=platform.profiles();profile=profiles.front();slots=platform.slots(profile);ensure(!slots.empty(),"No private UI test save");slot=slots.front();save=std::make_unique<Save>(platform.open(profile,slot));auto original=save->bytes();
            fieldList("Player",game.stats(*save));auto entries=rows();for(int i=0;i<30;++i)handle(Input{Down},entries);for(int i=0;i<30;++i)handle(Input{Up},entries);ensure(save->bytes()==original,"Navigation changed save data");
            push(View::Inventory,"Inventory");for(int i=0;i<9;++i)handle(Input{R},rows());ensure(save->bytes()==original,"Category navigation changed save data");
            auto item=game.items(*save).front();push(View::Item,metadata.itemName(item.id));current().item=item;auto detail=rows();for(int i=0;i<10;++i){handle(Input{Down},detail);handle(Input{Up},detail);}ensure(save->bytes()==original,"Detail navigation changed save data");
            // Undo from a BOTW replacement picker must discard its captured
            // item, just as it does for TOTK after insertion/removal.
            game.removeItem(*save,item);push(View::Picker,"Replace item");current().item=item;current().group=item.group;current().replacement=true;
            handle(Input{ZL},rows());ensure(current().view==View::Inventory&&save->bytes()==original,"BOTW undo retained a stale inventory screen");
            current().selected=0;current().scroll=0;
            confirm("Test","Cancel should be the default","Apply",[&]{throw Error("Confirmation accepted without choosing Apply");});handle(Input{A},rows());ensure(!dialog,"Cancel did not close dialog");
            std::cout<<"PASS: settings before save, single-user skip and back, save filters and selection mapping, multiple-user selection, loading error handling, navigation without edits, safe confirmation default\n";return 0;
        }
        if(argc>1&&std::string(argv[1])=="--screenshots"){
            std::filesystem::create_directories("preview");
            platform.settings.theme=Theme::Light;canvas.capturePath="preview/loading-light.png";loading("Getting things ready","Loading names and definitions from your installed game");
            platform.settings.theme=Theme::Oled;canvas.capturePath="preview/loading-oled.png";loading("Opening save","Profile 2 · Manual save");
            platform.settings.theme=Theme::Oled;push(View::Settings,"Settings");screenshot("preview/settings-oled.png");back();platform.settings.theme=Theme::Light;
            screenshot("preview/home-light.png");platform.settings.theme=Theme::Dark;screenshot("preview/home-dark.png");
            profiles=platform.profiles();profile=profiles.front();slots=platform.slots(profile);push(View::Slots,"Choose a save",profile.name);screenshot("preview/saves-dark.png");
            if(!slots.empty()){
                auto realSlots=slots;
                for(int i=1;i<14;++i){auto example=realSlots.front();example.gameProfile=i>=8?2:1;example.master=i==6||i==7;example.label=example.master?"Autosave · Master Mode":"Autosave";example.folder=i>=8?"ch_01/"+std::to_string(i-8):std::to_string(i);slots.push_back(example);}
                current().subtitle=profile.name+" · Newest first";screenshot("preview/saves-list-dark.png");
                current().selected=5;current().scroll=1;platform.settings.theme=Theme::Light;screenshot("preview/saves-list-light.png");
                changeSaveTab(1);platform.settings.theme=Theme::Oled;screenshot("preview/profile2-oled.png");changeSaveTab(2);screenshot("preview/master-oled.png");
                swapState.normal[0]=6;swapState.normal[1]=6;push(View::Swap,"Swap normal saves",profile.name);screenshot("preview/swap-oled.png");auto swapRows=rows();for(auto& row:swapRows)if(row.title.find("Master")!=std::string::npos||row.subtitle.find("Master")!=std::string::npos)throw Error("Master Mode appears in the swap menu");tryAction(swapRows.front().action);screenshot("preview/swap-confirm-oled.png");dialog.reset();back();
                slots=std::move(realSlots);changeSaveTab(0);platform.settings.theme=Theme::Dark;
            }
            if(!slots.empty()){slot=slots.front();save=std::make_unique<Save>(platform.open(profile,slot));fieldList("Player",game.stats(*save));screenshot("preview/player-dark.png");
                push(View::Inventory,"Inventory");screenshot("preview/inventory-dark.png");auto items=game.items(*save);if(!items.empty()){push(View::Item,metadata.itemName(items.front().id));current().item=items.front();screenshot("preview/detail-dark.png");}
                platform.settings.theme=Theme::Light;screenshot("preview/detail-light.png");}
            return 0;
        }
