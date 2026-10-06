// Copyright (c) 2026 StockiestThree9. MIT.
#include "totk.hpp"
#include "platform.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <set>
using namespace zse;
static unsigned checks=0;
static void require(bool ok,const char* why){++checks;if(!ok)throw Error(why);}
template<class F>static void refuses(F fn,const char* why){bool failed=false;try{fn();}catch(const std::exception&){failed=true;}require(failed,why);}
static void put(Bytes& b,size_t at,uint32_t v){for(unsigned i=0;i<4;++i)b.at(at+i)=uint8_t(v>>(i*8));}
static Bytes caption(){
 Bytes b(120,0);put(b,0,0x01020304);put(b,4,0x47e0f4);put(b,8,112);
 put(b,36,0);put(b,40,0x25f03caa);put(b,44,1);put(b,52,2);
 const uint32_t hashes[]={0x9811a3f7,0xdfd840d3,0xbd46f485,0x23f3d75e,0x27853bf7};const uint32_t values[]={2026,10,4,12,34};
 for(unsigned i=0;i<5;++i){put(b,56+i*8,hashes[i]);put(b,60+i*8,values[i]);}
 put(b,100,18);put(b,104,0x63696a32);put(b,108,112);put(b,112,4);b[116]=0xff;b[117]=0xd8;b[118]=0xff;b[119]=0xd9;return b;
}
#include "totk_inventory.hpp"
#include "totk_variants.hpp"
static void testNames(){
 // A tiny original MSBT inside a TOTK-style ActorMsg archive. No game text.
 const std::string label="Weapon_Sword_003_Name",shown="Example weapon";
 Bytes table(32);std::copy_n("MsgStdBn",8,table.begin());table[8]=0xff;table[9]=0xfe;table[14]=2;
 auto section=[&](const char* magic,const Bytes& payload){auto at=table.size();table.resize(at+16+payload.size());std::copy_n(magic,4,table.begin()+at);put(table,at+4,uint32_t(payload.size()));std::copy(payload.begin(),payload.end(),table.begin()+at+16);table.resize((table.size()+15)&~size_t(15));};
 Bytes labels(13+label.size()+4);put(labels,0,1);put(labels,4,1);put(labels,8,12);labels[12]=uint8_t(label.size());std::copy(label.begin(),label.end(),labels.begin()+13);section("LBL1",labels);
 Bytes text(8+shown.size()+1);put(text,0,1);put(text,4,8);std::copy(shown.begin(),shown.end(),text.begin()+8);section("TXT2",text);
 Bytes archive(96+table.size());std::copy_n("SARC",4,archive.begin());archive[4]=20;archive[6]=0xff;archive[7]=0xfe;put(archive,8,uint32_t(archive.size()));put(archive,12,96);
 std::copy_n("SFAT",4,archive.begin()+20);archive[24]=12;archive[26]=1;put(archive,36,0x01000000);put(archive,44,uint32_t(table.size()));std::copy_n("SFNT",4,archive.begin()+48);archive[52]=8;
 const std::string path="ActorMsg/Weapon.msbt";std::copy(path.begin(),path.end(),archive.begin()+56);std::copy(table.begin(),table.end(),archive.begin()+96);
 std::filesystem::create_directories("build/totk-resource-test/cache/totk");writeFile("build/totk-resource-test/cache/totk/messages.sarc",archive);
 Metadata data;loadTotkNames(data,"build/totk-resource-test",[](auto&){});require(data.names.size()==1&&data.itemName("Weapon_Sword_003")==shown,"TOTK ActorMsg item names were not loaded");
}
int main(int argc,char** argv){try{
 if(argc<2||argc>3)throw Error("Pass a local TOTK progress.sav fixture and optionally a private catalogue cache root. Neither is distributed.");
 auto original=readFile(argv[1]);Save save(original);require(save.game()==Game::Totk,"Wrong game detected");require(save.bytes()==original&&!save.dirty(),"Opening changed save bytes");
 testInventory(original);
 testNames();
 testVariantLinks();
 if(argc==3)testInstalledVariants(original,argv[2]);
 Metadata metadata;Totk model(metadata);model.definitions(save);auto stats=model.stats(save);auto get=[&](const char* name){auto it=std::find_if(stats.begin(),stats.end(),[&](auto& f){return f.field.name==name;});if(it==stats.end())throw Error(std::string("Missing stat: ")+name);return *it;};
 for(auto& field:stats)std::cout<<field.label<<": "<<displayValue(save,field)<<" "<<field.unit<<"\n";
 require(displayValue(save,get("PlayerStatus.CurrentRupee"))=="9142","Wrong fixture rupees");require(displayValue(save,get("PlayerStatus.MaxLife"))=="37","Hearts scaling failed");require(displayValue(save,get("PlayerStatus.MaxStamina"))=="2.2","Stamina scaling failed");require(displayValue(save,get("PlayerStatus.MaxEnergy"))=="39","Battery scaling failed");
 std::set<size_t> allowed;auto edit=[&](const EditField& f,const std::string& value){auto offset=save.fields().at(f.field.hash).at(f.index);for(unsigned i=0;i<4;++i)allowed.insert(offset+i);
 if(f.field.name.find(".Content.Effect.Type")!=std::string::npos){auto h=save.fieldHash(f.field.name.substr(0,f.field.name.size()-4)+"Value");if(save.count(h)>f.index){auto offset=save.fields().at(h).at(f.index);for(unsigned i=0;i<4;++i)allowed.insert(offset+i);}}
 setValue(save,f,value);};
 edit(get("PlayerStatus.CurrentRupee"),"12345");edit(get("PlayerStatus.MaxEnergy"),"48");edit(get("PlayerStatus.MaxStamina"),"3");
 auto all=model.items(save);require(!all.empty(),"No inventory parsed");size_t editedItems=0;
 std::array<unsigned,9> categories{};
 for(auto& item:all){if(categories[int(item.group)]>=3)continue;auto fields=model.itemFields(save,item);if(fields.empty())continue;++categories[int(item.group)];auto f=fields.front();if(!f.choices.empty())edit(f,std::to_string(f.choices.back().first));else edit(f,f.label=="Quantity"?"42":"111");++editedItems;
  for(auto& field:fields){auto shown=displayValue(save,field);require(!shown.empty(),"Item value missing");if(!field.choices.empty()){edit(field,std::to_string(field.choices.back().first));refuses([&]{setValue(save,field,"0");},"Unknown enum accepted");}}
 }
 require(editedItems>=20,"Insufficient inventory coverage");for(auto count:categories)require(count>0,"An inventory category lacks coverage");
 auto position=model.matching(save,[](auto& f){return f.name=="PlayerStatus.SavePos";});require(position.size()==1,"Saved position missing");auto axes=model.elements(save,position[0].field);require(axes.size()==3,"Position dimensions incorrect");edit(axes[0],"123.5");
 auto changed=save.bytes();size_t differences=0;for(size_t i=0;i<original.size();++i)if(changed[i]!=original[i]){++differences;require(allowed.count(i),"Untargeted save byte changed");}require(differences>0,"Edits had no effect");
 totk::Save reread(changed);require(reread.integer("PlayerStatus.CurrentRupee")==12345,"Edited rupees did not reparse");require(reread.real("PlayerStatus.MaxEnergy")==48000,"Edited battery scaling wrong");
 while(save.undo()){}require(save.bytes()==original&&!save.dirty(),"Undo did not restore byte-identical save");while(save.redo()){}require(save.bytes()==changed,"Redo differs from edited bytes");
 refuses([&]{setValue(save,get("PlayerStatus.MaxEnergy"),"49");},"Over-limit battery accepted");refuses([&]{setValue(save,get("PlayerStatus.CurrentRupee"),"1.5");},"Fractional rupees accepted");
 refuses([&]{save.edit("rollback",[&]{save.integer(save.fieldHash("PlayerStatus.CurrentRupee"),0,9);throw Error("Stop");});},"Transaction exception swallowed");require(save.bytes()==changed,"Failed transaction leaked edits");
 setValue(save,get("PlayerStatus.MaxLife"),"3");require(save.integer(save.fieldHash("PlayerStatus.Life"))<=12,"Current hearts exceed new maximum");refuses([&]{setValue(save,get("PlayerStatus.Life"),"4");},"Current life above maximum accepted");require(save.undo()&&save.bytes()==changed,"Heart edits did not undo atomically");
 auto malformed=original;put(malformed,8,33);refuses([&]{Save bad(malformed);},"Invalid table boundary accepted");malformed=original;totk::Save structure(original);auto names=structure.find("Pouch.Weapon.Content.Name");require(names,"Missing weapon array");put(malformed,names->offset-4,0xffffffff);refuses([&]{Save bad(malformed);},"Invalid array count accepted");
 Slot image;readTotkCaption(caption(),image);require(image.automatic&&image.image==Bytes({0xff,0xd8,0xff,0xd9})&&image.timestamp&&image.date.find("2026")!=std::string::npos,"Caption parsing failed");
 // All writes below are contained in a separate host fixture tree.
 auto work=std::filesystem::absolute("build/totk-storage-test");std::filesystem::create_directories(work);auto previous=std::filesystem::current_path();std::filesystem::current_path(work);
 std::filesystem::create_directories("host-data/totk/saves/slot_00");std::filesystem::create_directories("host-data/totk/saves/slot_01");std::filesystem::create_directories("host-data/saves/0");
 writeFile("host-data/totk/saves/slot_00/progress.sav",original);writeFile("host-data/totk/saves/slot_00/caption.sav",caption());writeFile("host-data/totk/saves/slot_01/progress.sav",original);Bytes botwSentinel={1,2,3};writeFile("host-data/saves/0/game_data.sav",botwSentinel);
 Platform platform;Profile profile{12,34,"Test user",{}};auto slots=platform.slots(profile,Game::Totk);require(slots.size()==2,"TOTK slots not found");auto found=std::find_if(slots.begin(),slots.end(),[](auto& s){return s.folder=="slot_00";});require(found!=slots.end()&&found->game==Game::Totk&&!found->master,"Wrong TOTK slot identity");
 auto opened=platform.open(profile,*found);require(opened.bytes()==original,"Opening slot modified data");auto backup=platform.commit(profile,*found,save);require(readFile(backup+"/progress.sav")==original,"TOTK backup differs");require(readFile(backup+"/caption.sav")==caption(),"TOTK caption not backed up");require(readFile("host-data/totk/saves/slot_00/progress.sav")==changed,"TOTK commit mismatch");require(!save.dirty(),"Commit not accepted");
 require(readFile("host-data/totk/saves/slot_01/progress.sav")==original,"Another TOTK slot changed");require(readFile("host-data/saves/0/game_data.sav")==botwSentinel,"BOTW data changed");
 setValue(save,get("PlayerStatus.CurrentRupee"),"7");writeFile("host-data/totk/saves/slot_00/progress.sav",original);refuses([&]{platform.commit(profile,*found,save);},"Stale TOTK save accepted");require(readFile("host-data/totk/saves/slot_00/progress.sav")==original,"Stale conflict overwrote save");Slot wrong=*found;wrong.game=Game::Botw;refuses([&]{platform.commit(profile,wrong,save);},"Cross-game commit allowed");
 std::filesystem::current_path(previous);std::cout<<"PASS: "<<checks<<" TOTK checks; real fixture, field scaling, targeted writes, enum choices, undo/redo, malformed data, caption, backups, isolation and conflicts\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
