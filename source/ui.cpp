// Copyright (c) 2026 StockiestThree9
#include "ui.hpp"
#include "totk.hpp"
#include <SDL_image.h>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <deque>
#include <functional>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <atomic>
#include <exception>

namespace zse {
namespace {
struct Palette { SDL_Color bg,panel,text,muted,line,accent,focus,danger; };
Palette palette(bool dark){return dark?Palette{{45,45,45,255},{55,55,55,255},{246,246,246,255},{181,181,181,255},{89,89,89,255},{0,227,201,255},{34,62,62,255},{255,139,137,255}}:Palette{{235,235,235,255},{249,249,249,255},{42,42,42,255},{109,109,109,255},{199,199,199,255},{0,142,143,255},{222,245,241,255},{175,45,45,255}};}
Palette oledPalette(){return {{0,0,0,255},{17,17,17,255},{246,246,246,255},{181,181,181,255},{62,62,62,255},{0,227,201,255},{8,38,35,255},{255,139,137,255}};}
class Canvas {
public:
    Platform& platform;
    SDL_Window* window=nullptr;
    SDL_Renderer* renderer=nullptr;
    Palette p=palette(false);
    std::map<std::string,SDL_Texture*> images;
    struct TextTexture {SDL_Texture* texture;int w,h;};
    std::map<std::string,TextTexture> textCache;
    std::string capturePath;
    explicit Canvas(Platform& plat):platform(plat){
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"linear");
        window=SDL_CreateWindow("Zelda Save Editor",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1280,720,SDL_WINDOW_SHOWN);
        if(!window)throw Error(SDL_GetError());
        renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
        if(!renderer)renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
        if(!renderer)throw Error(SDL_GetError());
        SDL_RenderSetLogicalSize(renderer,1280,720);SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
#ifdef __SWITCH__
        images["logo"]=IMG_LoadTexture(renderer,"romfs:/logo.jpg");
#else
        images["logo"]=IMG_LoadTexture(renderer,"romfs/logo.jpg");
#endif
    }
    ~Canvas(){for(auto& t:textCache)SDL_DestroyTexture(t.second.texture);for(auto& i:images)SDL_DestroyTexture(i.second);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);}
    void rect(int x,int y,int w,int h,SDL_Color c){SDL_SetRenderDrawColor(renderer,c.r,c.g,c.b,c.a);SDL_Rect r{x,y,w,h};SDL_RenderFillRect(renderer,&r);}
    void border(int x,int y,int w,int h,SDL_Color c,int thick=3){rect(x,y,w,thick,c);rect(x,y+h-thick,w,thick,c);rect(x,y,thick,h,c);rect(x+w-thick,y,thick,h,c);}
    void line(int x,int y,int xx,int yy,SDL_Color c){SDL_SetRenderDrawColor(renderer,c.r,c.g,c.b,c.a);SDL_RenderDrawLine(renderer,x,y,xx,yy);}
    void text(const std::string& value,int x,int y,int size,SDL_Color color,int width=1200,bool right=false){
        if(value.empty()||width<=0)return;
        std::string key=std::to_string(size)+"/"+std::to_string(width)+"/"+std::to_string(color.r)+"/"+std::to_string(color.g)+"/"+std::to_string(color.b)+"/"+value;
        auto it=textCache.find(key);
        if(it==textCache.end()){
            if(textCache.size()>350){for(auto& t:textCache)SDL_DestroyTexture(t.second.texture);textCache.clear();}
            std::string clipped=value;int tw=0,th=0;TTF_SizeUTF8(platform.font(size),clipped.c_str(),&tw,&th);
            if(tw>width){while(!clipped.empty()){size_t end=clipped.size()-1;while(end>0&&(static_cast<unsigned char>(clipped[end])&0xc0)==0x80)--end;clipped.resize(end);std::string v=clipped+"…";TTF_SizeUTF8(platform.font(size),v.c_str(),&tw,&th);if(tw<=width){clipped=v;break;}}}
            auto surface=TTF_RenderUTF8_Blended(platform.font(size),clipped.c_str(),color);if(!surface)return;
            auto texture=SDL_CreateTextureFromSurface(renderer,surface);TextTexture result{texture,surface->w,surface->h};SDL_FreeSurface(surface);
            it=textCache.emplace(key,result).first;
        }
        auto& t=it->second;SDL_Rect dest{x+(right?width-t.w:0),y,t.w,t.h};SDL_Rect clip{x,y,width,80};SDL_RenderSetClipRect(renderer,&clip);SDL_RenderCopy(renderer,t.texture,nullptr,&dest);SDL_RenderSetClipRect(renderer,nullptr);
    }
    void wrapped(const std::string& value,int x,int y,int width,int size,SDL_Color color,int maxLines=5){
        std::istringstream words(value);std::string word,lineText;int lineNum=0;
        while(words>>word){std::string next=lineText.empty()?word:lineText+" "+word;int tw=0;TTF_SizeUTF8(platform.font(size),next.c_str(),&tw,nullptr);
            if(tw>width&&!lineText.empty()){text(lineText,x,y+lineNum*(size+10),size,color,width);if(++lineNum>=maxLines)return;lineText=word;}else lineText=next;}
        if(!lineText.empty()&&lineNum<maxLines)text(lineText,x,y+lineNum*(size+10),size,color,width);
    }
    void imageBytes(const std::string& key,const Bytes& bytes,int x,int y,int w,int h){
        if(images.find(key)==images.end()){SDL_Texture* t=nullptr;if(!bytes.empty())t=IMG_LoadTexture_RW(renderer,SDL_RWFromConstMem(bytes.data(),int(bytes.size())),1);images[key]=t;}
        auto texture=images[key];if(texture){int tw=0,th=0;SDL_QueryTexture(texture,nullptr,nullptr,&tw,&th);double scale=std::min(double(w)/tw,double(h)/th);int dw=int(tw*scale),dh=int(th*scale);SDL_Rect dst{x+(w-dw)/2,y+(h-dh)/2,dw,dh};SDL_RenderCopy(renderer,texture,nullptr,&dst);}else rect(x,y,w,h,p.line);
    }
    void logo(int x,int y,int size){if(images["logo"]){SDL_Rect r{x,y,size,size};SDL_RenderCopy(renderer,images["logo"],nullptr,&r);}}
    void centered(const std::string& value,int y,int size,SDL_Color color,int width=1000){int w=0;TTF_SizeUTF8(platform.font(size),value.c_str(),&w,nullptr);text(value,(1280-std::min(w,width))/2,y,size,color,width);}
    void spinner(int x,int y,uint64_t tick){
        int head=int(tick/75)%12;
        for(int i=0;i<12;++i){double a=(i-3)*3.141592653589793/6;int age=(head-i+12)%12;double alpha=0.16+0.84*(11-age)/11;
            SDL_Color color{Uint8(p.bg.r+(p.text.r-p.bg.r)*alpha),Uint8(p.bg.g+(p.text.g-p.bg.g)*alpha),Uint8(p.bg.b+(p.text.b-p.bg.b)*alpha),255};
            for(int w=-2;w<=2;++w){int dx=int(-std::sin(a)*w),dy=int(std::cos(a)*w);line(x+int(std::cos(a)*22)+dx,y+int(std::sin(a)*22)+dy,x+int(std::cos(a)*34)+dx,y+int(std::sin(a)*34)+dy,color);}
        }
    }
    void start(){p=platform.settings.theme==Theme::Oled?oledPalette():palette(platform.dark());SDL_SetRenderDrawColor(renderer,p.bg.r,p.bg.g,p.bg.b,255);SDL_RenderClear(renderer);}
    void present(){if(!capturePath.empty()){shot(capturePath);capturePath.clear();}SDL_RenderPresent(renderer);}
    void shot(const std::string& path){auto s=SDL_CreateRGBSurfaceWithFormat(0,1280,720,32,SDL_PIXELFORMAT_RGBA32);SDL_RenderReadPixels(renderer,nullptr,s->format->format,s->pixels,s->pitch);IMG_SavePNG(s,path.c_str());SDL_FreeSurface(s);}
};
enum class View { Games,Profiles,Slots,Sections,Inventory,Item,Picker,Horses,Fields,Settings,Completion,Modifiers,Swap,Choices };
enum class SaveTab {Normal,Profile2,Master,Other};
struct Frame {
    View view=View::Games;
    std::string title,subtitle,filter;
    std::vector<EditField> fields;
    int selected=0,scroll=0;
    ItemGroup group=ItemGroup::Weapons;
    Item item;
    EditField choice;
    bool expanded=false,replacement=false;
    SaveTab saveTab=SaveTab::Normal;
};
struct Row {std::string title,subtitle,value;std::function<void()> action;bool danger=false;};
struct Dialog {std::string title,message,accept="OK";std::function<void()> action;bool confirm=false;int selected=0;};
class App {
public:
    Platform platform;
    Canvas canvas;
    Metadata metadata;
    Botw game;
    Totk tears;
    Game selectedGame=Game::Botw;
    std::unique_ptr<Save> save;
    std::vector<Profile> profiles;
    std::vector<Slot> slots;
    Profile profile;
    Slot slot;
    SwapInfo swapState;
    std::vector<Frame> stack;
    std::optional<Dialog> dialog;
    bool running=true,dirty=true,rebuild=true;
    std::string toast;
    uint64_t toastUntil=0;
    App():canvas(platform),game(metadata),tears(metadata){Frame f;f.title="Zelda Save Editor";f.subtitle="0.1.5 TOTK alpha 2";stack.push_back(f);}
    bool isTotk() const {return selectedGame==Game::Totk;}
    std::vector<Item> items() const {return isTotk()?tears.items(*save):game.items(*save);}
    std::string sectionName(ItemGroup group) const {return isTotk()&&group==ItemGroup::Unknown?"Zonai devices":groupName(group);}
    void selectGame(Game selected){
        save.reset();selectedGame=selected;metadata={};
        busy("Reading game resources",gameName(selected),[&]{platform.loadMetadata(metadata,selected);});
        for(auto it=canvas.images.begin();it!=canvas.images.end();){if(it->first.rfind("slot",0)==0){SDL_DestroyTexture(it->second);it=canvas.images.erase(it);}else ++it;}
        chooseUser(platform.profiles());
    }
    Frame& current(){return stack.back();}
    void push(View view,const std::string& title,const std::string& subtitle=""){Frame f;f.view=view;f.title=title;f.subtitle=subtitle;stack.push_back(std::move(f));dirty=true;rebuild=true;}
    void notice(const std::string& s){toast=s;toastUntil=SDL_GetTicks64()+3500;dirty=true;}
    void message(const std::string& title,const std::string& body){dialog=Dialog{title,body,"OK",{},false,0};dirty=true;}
    void confirm(const std::string& title,const std::string& body,const std::string& accept,std::function<void()> action){dialog=Dialog{title,body,accept,std::move(action),true,0};dirty=true;}
    void tryAction(const std::function<void()>& action){try{if(action)action();}catch(const std::exception& e){platform.log(e.what());platform.cue(Cue::Error);message("Could not complete that",e.what());}dirty=true;rebuild=true;}
    void selectProfile(const Profile& p){
        profile=p;std::vector<Slot> found;busy("Finding your saves",p.name,[&]{swapState=isTotk()?SwapInfo{}:platform.swapInfo(p);if(!swapState.pending)found=platform.slots(p,selectedGame);});
        slots=std::move(found);push(View::Slots,"Choose a save",p.name+" · Newest first");if(swapState.pending)push(View::Swap,"Recover profile swap",p.name);
    }
    static bool inTab(const Slot& s,SaveTab tab){if(tab==SaveTab::Master)return s.master;if(tab==SaveTab::Normal)return s.gameProfile==1&&!s.master;if(tab==SaveTab::Profile2)return s.gameProfile==2&&!s.master;return !s.gameProfile;}
    std::vector<const Slot*> filteredSlots() const {std::vector<const Slot*> out;for(auto& s:slots)if(inTab(s,stack.back().saveTab))out.push_back(&s);return out;}
    int tabCount() const {if(isTotk())return 1;return std::any_of(slots.begin(),slots.end(),[](auto& s){return !s.gameProfile;})?4:3;}
    void changeSaveTab(int tab){current().saveTab=SaveTab(tab);current().selected=0;current().scroll=0;platform.cue(Cue::Focus);dirty=true;rebuild=true;}
    void manageProfiles(){busy("Reading normal saves",profile.name,[&]{swapState=platform.swapInfo(profile);});push(View::Swap,swapState.pending?"Recover profile swap":"Swap normal saves",profile.name);}
    void exchange(bool resume){
        std::string backup;try{busy(resume?"Resuming normal-save swap":"Swapping normal saves","Verifying both sets and preserving your backup. Keep the app open.",[&]{backup=platform.swapProfiles(profile,resume);});}
        catch(...){try{swapState=platform.swapInfo(profile);if(swapState.pending)current().title="Recover profile swap";}catch(...){}throw;}
        busy("Refreshing your saves",profile.name,[&]{slots=platform.slots(profile);swapState=platform.swapInfo(profile);});
        while(stack.size()>1&&current().view!=View::Slots)stack.pop_back();current().saveTab=SaveTab::Normal;current().selected=0;current().scroll=0;
        for(auto it=canvas.images.begin();it!=canvas.images.end();){if(it->first.rfind("slot",0)==0){SDL_DestroyTexture(it->second);it=canvas.images.erase(it);}else ++it;}
        message("Normal saves swapped","Both normal adventures are retained. The set in Profile 1 is now active. Swap again to switch back. Full backup: "+backup);
    }
    void chooseUser(std::vector<Profile> users){
        profiles=std::move(users);
        if(profiles.empty())throw Error("No console users were found. Create a user in System Settings, then reopen the editor.");
        if(profiles.size()==1)selectProfile(profiles.front());else push(View::Profiles,"Choose a user",gameName(selectedGame));
    }
    void back(){if(stack.size()==1){running=false;return;}if(current().view==View::Sections&&save&&save->dirty()){confirm("Discard changes?","Your edits have not been written to the game. Going back will discard them.","Discard",[this]{save.reset();stack.pop_back();});return;}if(current().view==View::Sections)save.reset();stack.pop_back();platform.cue(Cue::Back);dirty=true;rebuild=true;}
    void fieldList(const std::string& title,std::vector<EditField> fields,const std::string& subtitle=""){push(View::Fields,title,subtitle);current().fields=std::move(fields);}
    void editField(EditField field){
        if(!field.choices.empty()){push(View::Choices,field.label,"Choose an option");current().choice=field;for(size_t i=0;i<field.choices.size();++i)if(field.choices[i].first==save->word(field.field.hash,field.index))current().selected=int(i);current().scroll=std::max(0,current().selected-5);return;}
        if(field.field.kind==Kind::Bool){setValue(*save,field,save->word(field.field.hash,field.index)?"0":"1");platform.cue(Cue::Confirm);return;}
        bool text=field.field.kind==Kind::String;auto result=platform.keyboard(field.label,displayValue(*save,field),text?field.field.width*4-1:24,!text,!field.whole);
        if(result){setValue(*save,field,*result);platform.cue(Cue::Confirm);}
    }
    void chooseField(EditField f){size_t n=save->count(f.field.hash)/(f.field.kind==Kind::String?f.field.width:1);
        if(!current().expanded&&n>1){fieldList(f.label,isTotk()?tears.elements(*save,f.field):game.elements(*save,f.field),f.field.name);current().expanded=true;}else editField(f);
    }
    void review(){
        if(!save||!save->dirty()){notice("No unsaved changes");return;}
        confirm("Save changes?",std::to_string(save->changedWords())+" values will be written to "+profile.name+"'s "+slot.identity()+". An exact backup of this slot will be verified first.","Save changes",[this]{
            std::string path;busy("Saving changes","Backing up and verifying this save. Please keep the app open.",[&]{path=platform.commit(profile,slot,*save);});message("Changes saved","Your original save is backed up at "+path);});
    }
    void search(){
        auto input=platform.keyboard("Search",current().filter,80);if(input){current().filter=*input;current().selected=0;current().scroll=0;dirty=true;}
    }
    bool matches(const std::string& value) const {if(stack.back().filter.empty())return true;auto lower=[](std::string s){for(char& c:s)c=char(std::tolower(static_cast<unsigned char>(c)));return s;};return lower(value).find(lower(stack.back().filter))!=std::string::npos;}
    std::string shown(const EditField& f) const {auto value=displayValue(*save,f);if(f.unit.empty())return value;std::string unit=f.unit;if(value=="1"&&(unit=="wheels"||unit=="hearts"||unit=="seconds"||unit=="rupees"))unit.pop_back();return value+(unit=="%"?"":" ")+unit;}
    std::string location(const Slot& s) const {auto it=metadata.locations.find(s.location);return it!=metadata.locations.end()?it->second:s.location.empty()?(s.game==Game::Totk?"Save slot "+s.folder.substr(5):"Unknown location"):readable(s.location);}
    std::vector<Row> rows(){
        std::vector<Row> r;auto view=current().view;
        if(save&&(view==View::Item||view==View::Modifiers)){
            auto all=items();auto item=std::find_if(all.begin(),all.end(),[&](auto& value){return value.index==current().item.index;});
            if(item==all.end()){while(stack.size()>1&&current().view!=View::Inventory)stack.pop_back();return rows();}
            current().item=*item;if(view==View::Item)current().title=metadata.itemName(item->id);
        }
        if(view==View::Games){
            r.push_back({"Breath of the Wild","Nintendo Switch","Open",[this]{selectGame(Game::Botw);}});
            r.push_back({"Tears of the Kingdom","Nintendo Switch · Experimental support","Open",[this]{selectGame(Game::Totk);}});
        }
        else if(view==View::Profiles){for(auto p:profiles)r.push_back({p.name,std::string(gameName(selectedGame))+" save data","",[this,p]{selectProfile(p);}});}
        else if(view==View::Slots){for(auto entry:filteredSlots()){auto s=*entry;r.push_back({s.identity(),location(s),s.date,[this,s]{std::unique_ptr<Save> opened;busy("Opening save",s.identity(),[&]{opened=std::make_unique<Save>(platform.open(profile,s));});slot=s;save=std::move(opened);if(isTotk())tears.definitions(*save);push(View::Sections,gameName(selectedGame),profile.name+" · "+slot.identity());}});}}
        else if(view==View::Swap){
            if(swapState.pending)r.push_back({"Resume interrupted swap","Finish moving the retained data into its intended locations","Resume",[this]{confirm("Resume the swap?","Keep BOTW closed. The editor will verify the retained data and continue the interrupted operation.","Resume",[this]{exchange(true);});}});
            else r.push_back({"Swap Profile 1 and Profile 2","Exchange the two normal adventures","Review",[this]{confirm("Swap normal saves?","Keep BOTW closed. Normal saves, their thumbnails and normal Hero's Path will exchange places. Shared photo libraries stay at their current locations. A full backup is verified first.","Swap saves",[this]{exchange(false);});}});
            r.push_back({"Profile 1 · Active location",std::to_string(swapState.normal[0])+" normal saves","",[this]{message("Profile 1","This is the normal-save location used by Breath of the Wild on this Switch. Swapping puts the other normal adventure here.");}});
            r.push_back({"Profile 2 · Inactive location",std::to_string(swapState.normal[1])+" normal saves","",[this]{message("Profile 2","The second profile was introduced with the Switch 2 edition. Saves stored here are editable but cannot be played in this location on this Switch.");}});
            r.push_back({"What moves?","Normal saves, save thumbnails and normal Hero's Path","Details",[this]{message("Normal-save swap","Each set's six normal save slots, their save pictures and normal Hero's Path move together. Albums and compendium pictures stay in place, so these images may differ after a swap. All originals are retained in the verified full backup.");}});
        }
        else if(view==View::Sections&&isTotk()){
            r.push_back({"Player","Hearts, stamina wheels, battery cells and rupees","",[this]{fieldList("Player",tears.stats(*save));current().expanded=true;}});
            r.push_back({"Inventory","Add, replace, duplicate and customize items","",[this]{push(View::Inventory,"Inventory",metadata.names.empty()?"Item IDs shown · Reload game resources in Settings":"L / R changes section");}});
            r.push_back({"Saved position","Link's saved coordinates","",[this]{fieldList("Saved position",tears.matching(*save,[](auto& f){return f.name=="PlayerStatus.SavePos";}),"Coordinates apply in the currently saved map");}});
            r.push_back({"About TOTK support","Experimental · Player and inventory editing","Details",[this]{message("TOTK alpha","Manage your ordinary inventory, equipment, meals and player values. Temporary challenge inventory is preserved. Key item creation, fusion replacement, horses, quests and world completion are still in development. Saving requires confirmation and a verified backup.");}});
        }else if(view==View::Choices){
            auto field=current().choice;for(auto option:field.choices)r.push_back({option.second,"",save->word(field.field.hash,field.index)==option.first?"Selected":"",[this,field,option]{setValue(*save,field,std::to_string(option.first));back();platform.cue(Cue::Confirm);}});
        }else if(view==View::Sections){
            r.push_back({"Player","Hearts, stamina, rupees and abilities","",[this]{fieldList("Player",game.stats(*save));}});
            r.push_back({"Inventory","Equipment, materials, meals and key items","",[this]{push(View::Inventory,"Inventory","L / R changes section");}});
            r.push_back({"Horses","Registered horses and the wild horse slot","",[this]{push(View::Horses,"Horses");}});
            r.push_back({"World & completion","Position, weather, shrines and collectibles","",[this]{push(View::Completion,"World & completion");}});
            r.push_back({"Amiibo","Manage scan history","",[this]{fieldList("Amiibo",game.matching(*save,[](auto& f){return f.name.find("Amiibo")==0;}));}});
            r.push_back({"Advanced","Search every named field in your save","",[this]{fieldList("Advanced",game.matching(*save,[](auto&){return true;}),"Direct field editing · Related flags may need matching changes");}});
        }else if(view==View::Settings){
            const char* themes[]={"Automatic","Light","Dark","OLED black"};std::string theme=themes[int(platform.settings.theme)];
            r.push_back({"Appearance",platform.settings.theme==Theme::Oled?"Pure black backgrounds for OLED screens":"Automatic follows your Switch theme",theme,[this]{platform.settings.theme=Theme((int(platform.settings.theme)+1)%4);platform.saveSettings();}});
            r.push_back({"Navigation sounds",platform.nativeSounds?"Loaded from this Switch's HOME menu":"System sounds are unavailable in this session",platform.settings.sound?"On":"Off",[this]{platform.settings.sound=!platform.settings.sound;platform.saveSettings();platform.cue(Cue::Confirm);}});
            r.push_back({"About","Zelda Save Editor · 0.1.5 TOTK alpha 2","StockiestThree9",[this]{message("Zelda Save Editor","Created by StockiestThree9. Native editor code written for this project. Uses libnx, SDL, FreeType, HarfBuzz and libpulsar. Feature reference: Marc Robledo's web save editor. Format reference: ZeldaMods. Nintendo sounds, fonts and game data are read from your console.");}});
            r.push_back({"Game resources",platform.resourceStatus,"Reload",[this]{busy("Reading game resources","Loading names and definitions from your installed game",[&]{metadata={};platform.loadMetadata(metadata,selectedGame);if(save&&isTotk())tears.definitions(*save);});notice(platform.resourceStatus);}});
        }else if(view==View::Fields){
            for(auto field:current().fields){if(!matches(field.label+" "+field.field.name))continue;size_t n=save->count(field.field.hash)/(field.field.kind==Kind::String?field.field.width:1);
                std::string value=!current().expanded&&n>1?std::to_string(n)+" entries":shown(field);
                r.push_back({field.label,field.description,value,[this,field]{chooseField(field);}});
            }
            if(current().title=="Amiibo"&&current().filter.empty())r.insert(r.begin(),{"Reset today's scans","Keeps your total scan history","",[this]{confirm("Reset amiibo scans?","Clear today's scan history so amiibo can be scanned again.","Reset scans",[this]{game.resetAmiibo(*save);notice("Daily scan history cleared");});}});
        }else if(view==View::Inventory){
            for(auto item:items()){if(item.group!=current().group||!matches(metadata.itemName(item.id)+" "+item.id))continue;
                if(isTotk()){r.push_back({metadata.itemName(item.id),tears.itemDetail(*save,item),tears.itemValue(*save,item),[this,item]{push(View::Item,metadata.itemName(item.id),tears.itemDetail(*save,item));current().item=item;}});continue;}
                int value=save->integer(hashName("PorchItem_Value1"),item.index);bool equipment=item.group==ItemGroup::Weapons||item.group==ItemGroup::Bows||item.group==ItemGroup::Shields;
                std::string valueLabel=equipment?std::to_string(value/100)+" durability":item.group==ItemGroup::Armor?(value?"Dyed":"Original color"):"× "+std::to_string(value);
                r.push_back({metadata.itemName(item.id),save->word(hashName("PorchItem_EquipFlag"),item.index)?"Equipped":groupName(item.group),valueLabel,[this,item]{push(View::Item,metadata.itemName(item.id),groupName(item.group));current().item=item;}});
            }
            if(!isTotk()||tears.canManage(current().group))r.push_back({"Add item","Choose from your game's item catalogue",isTotk()?std::to_string(tears.availableSlots(*save,current().group))+" free":"+",[this]{auto group=current().group;bool equipment=group==ItemGroup::Weapons||group==ItemGroup::Bows||group==ItemGroup::Shields;push(View::Picker,"Add item",isTotk()&&equipment?"New equipment starts at 1 durability · Adjust it after adding":sectionName(group));current().group=group;}});
            if(isTotk()&&(current().group==ItemGroup::Materials||current().group==ItemGroup::Arrows||current().group==ItemGroup::Unknown))r.push_back({"Fill stacks","Set the quantity of every owned item in this section to 999","",[this]{auto group=current().group;confirm("Fill these stacks?","All owned "+sectionName(group)+" stacks in the ordinary pouch will be set to 999. This can be undone before saving.","Fill stacks",[this,group]{tears.fillStacks(*save,group);notice("Stacks filled");});}});
        }else if(view==View::Item){auto item=current().item;
            if(isTotk()&&tears.itemFields(*save,item).empty())r.push_back({"Owned item","No editable quantity is stored for this item","",[this]{message("Owned item","This item uses an ownership marker. Quantity editing is unavailable for it.");}});
            if(!isTotk()||tears.canManage(item))r.push_back({"Replace item",isTotk()?"Keeps this item's values and fused material":"Choose a different item in this section","Choose",[this,item]{push(View::Picker,"Replace item",sectionName(item.group));current().group=item.group;current().item=item;current().replacement=true;}});
            if(isTotk()&&(item.group==ItemGroup::Weapons||item.group==ItemGroup::Bows||item.group==ItemGroup::Shields||item.group==ItemGroup::Armor))r.push_back({"Equipped","Changes the ordinary pouch only",tears.equipped(*save,item)?"Yes":"No",[this,item]{tears.equipItem(*save,item);}});
            for(auto f:(isTotk()?tears.itemFields(*save,item):game.itemFields(*save,item))){
                if(f.label=="Modifier flags")r.push_back({"Modifier","Choose an equipment bonus","Choose",[this,item]{push(View::Modifiers,"Equipment modifier");current().item=item;}});
                else if(f.label=="Equipped")r.push_back({f.label,"",displayValue(*save,f),[this,item]{game.equipItem(*save,item);}});
                else r.push_back({f.label,f.description,shown(f),[this,f]{editField(f);}});
            }
            if(isTotk()&&tears.canManage(item)&&tears.canDuplicate(item.group))r.push_back({"Duplicate item","Copies its values, fuse data and meal ingredients","",[this,item]{tears.duplicateItem(*save,item);notice("Item duplicated");}});
            if(isTotk()&&tears.itemDetail(*save,item).rfind("Fused: ",0)==0)r.push_back({"Remove fused material","Keeps the base item and its current durability","",[this,item]{confirm("Remove fused material?","The attachment and its fuse bonus will be removed. The base item stays in your pouch.","Remove fuse",[this,item]{tears.clearFusion(*save,item);current().subtitle=tears.itemDetail(*save,item);notice("Fused material removed");});}});
            if(!isTotk()||tears.canManage(item))r.push_back({"Remove item","This can be undone before saving","",[this,item]{confirm("Remove this item?",metadata.itemName(item.id),"Remove",[this,item]{if(isTotk())tears.removeItem(*save,item);else game.removeItem(*save,item);stack.pop_back();notice("Item removed");});},true});
        }else if(view==View::Modifiers){
            auto item=current().item;EditField flags;for(auto f:game.itemFields(*save,item))if(f.label=="Modifier flags")flags=f;
            uint32_t currentValue=save->word(flags.field.hash,flags.index);
            const std::pair<uint32_t,const char*> options[]={{0,"None"},{1,"Attack up"},{2,"Durability up"},{4,"Critical hit"},{8,"Long throw"},{16,"Multiple arrows"},{32,"Long range"},{64,"Quick shot"},{128,"Shield surf"}};
            for(auto option:options){if((option.first==16||option.first==32||option.first==64)&&item.group!=ItemGroup::Bows)continue;if(option.first==128&&item.group!=ItemGroup::Shields)continue;if((option.first==4||option.first==8)&&item.group!=ItemGroup::Weapons)continue;
                r.push_back({option.second,"",(currentValue&0x7fffffff)==option.first?"Selected":"",[this,flags,option,currentValue]{uint32_t next=option.first?option.first|(currentValue&0x80000000u):0;setValue(*save,flags,std::to_string(next));notice("Modifier updated");}});
            }
            r.push_back({"Gold tier","Enhanced equipment bonus",currentValue&0x80000000u?"On":"Off",[this,flags,currentValue]{setValue(*save,flags,std::to_string(currentValue^0x80000000u));}});
        }else if(view==View::Picker&&isTotk()){
            for(auto id:tears.catalog(*save,current().group)){
                if(!matches(id+" "+metadata.itemName(id)))continue;
                if(current().replacement&&current().group==ItemGroup::Armor&&id.substr(id.rfind('_')+1)!=current().item.id.substr(current().item.id.rfind('_')+1))continue;
                r.push_back({metadata.itemName(id),id,"",[this,id]{
                    if(current().replacement){auto item=current().item;tears.replaceItem(*save,item,id);stack.pop_back();current().item.id=id;current().title=metadata.itemName(id);current().subtitle=tears.itemDetail(*save,current().item);}
                    else{auto group=current().group;auto item=tears.addItem(*save,group,id);stack.pop_back();push(View::Item,metadata.itemName(id),tears.itemDetail(*save,item));current().item=item;}
                    notice("Inventory updated");
                }});
            }
            std::sort(r.begin(),r.end(),[](auto& a,auto& b){return a.title<b.title;});
        }else if(view==View::Picker){
            for(auto& pair:metadata.names){if(itemGroup(pair.first)!=current().group||!matches(pair.first+" "+pair.second))continue;
                // A pickup flag or inventory timestamp verifies that this is an inventory actor.
                if(!metadata.fields.count(hashName("IsGet_"+pair.first))&&!metadata.fields.count(hashName("PorchTime_"+pair.first)))continue;
                auto id=pair.first;r.push_back({pair.second,id,"",[this,id]{if(current().replacement){auto item=current().item;game.replaceItem(*save,item,id);stack.pop_back();current().item.id=id;current().title=metadata.itemName(id);}else{game.addItem(*save,id);stack.pop_back();}notice("Inventory updated");}});
            }
            std::sort(r.begin(),r.end(),[](auto& a,auto& b){return a.title<b.title;});
        }else if(view==View::Horses){for(size_t i=0;i<save->count("Horse_ActorName")/16;++i){auto name=save->string(hashName("Horse_UserName"),64,i);auto actor=save->string(hashName("Horse_ActorName"),64,i);
            r.push_back({i==5?"Wild horse":name.empty()?"Horse "+std::to_string(i+1):name,actor.empty()?"Empty slot":actor,"",[this,i]{fieldList(i==5?"Wild horse":"Horse "+std::to_string(i+1),game.horseFields(*save,i));current().expanded=true;}});}
        }else if(view==View::Completion){
            auto list=[this](const std::string& name,std::function<bool(const Field&)> filter){fieldList(name,game.matching(*save,filter));};
            r.push_back({"Link's position","Coordinates and saved map","",[list]{list("Link's position",[](auto& f){return f.name.find("PlayerSavePos")==0;});}});
            r.push_back({"Weather","Regional forecasts","",[list]{list("Weather",[](auto& f){return f.name.find("climateWeather")==0;});}});
            r.push_back({"Koroks","Individual discovery flags","",[list]{list("Koroks",[](auto& f){return f.name.find("MainField_Npc_HiddenKorok")==0;});}});
            r.push_back({"Shrines","Discovery, entry and completion flags","",[list]{list("Shrines",[](auto& f){return f.name.find("Open_Dungeon")==0||f.name.find("Clear_Dungeon")==0||f.name.find("Enter_Dungeon")==0;});}});
            r.push_back({"Locations & towers","Map discovery","",[list]{list("Locations & towers",[](auto& f){return f.name.find("Location_")==0||f.name.find("MapTower_")==0;});}});
            r.push_back({"Compendium","Picture book entries","",[list]{list("Compendium",[](auto& f){return f.name.find("PictureBook")==0||f.name.find("IsRegisteredPictureBook")==0;});}});
            r.push_back({"Defeated enemies","Enemy counts and boss flags","",[list]{list("Defeated enemies",[](auto& f){return f.name.find("Defeated_")==0||f.name.find("IsDefeat_Enemy_")==0;});}});
            r.push_back({"Map stamps","Positions and icon types","",[list]{list("Map stamps",[](auto& f){return f.name.find("MapApp_MapIcon")==0;});}});
            r.push_back({"Quests","Quest activation and completion flags","",[list]{list("Quests",[](auto& f){return f.name.size()>7&&(f.name.compare(f.name.size()-7,7,"_Finish")==0||(f.name.size()>10&&f.name.compare(f.name.size()-10,10,"_Activated")==0));});}});
        }
        return r;
    }
    void footer(bool modal=false){
        auto& c=canvas;c.rect(40,650,1200,1,c.p.line);
        c.text("StockiestThree9",48,676,18,c.p.muted,250);
        if(modal){c.text("B  Cancel     A  Select",850,670,24,c.p.text,370,true);return;}
        std::string controls=current().view==View::Games?"X  Settings    B  Exit    A  Select":"X  Settings    B  Back    A  Select";
        if(current().view==View::Settings)controls="B  Back    A  Select";
        if(current().view==View::Slots&&!isTotk())controls=current().saveTab==SaveTab::Normal||current().saveTab==SaveTab::Profile2?"L / R  Tabs    Y  Swap saves    X  Settings    B  Back    A  Select":"L / R  Tabs    X  Settings    B  Back    A  Select";
        if(save&&current().view!=View::Settings){controls="ZL  Undo    ZR  Redo    ";if(current().view==View::Fields||current().view==View::Inventory||current().view==View::Picker)controls+="Y  Search    ";controls+="+  Save    X  Settings    B  Back    A  Select";}
        c.text(controls,330,670,21,c.p.text,890,true);
    }
    void loading(const std::string& title,const std::string& sub){
        auto& c=canvas;c.start();c.logo(42,33,48);c.text("Zelda Save Editor",110,42,32,c.p.text,1000);c.rect(40,112,1200,1,c.p.line);
        c.spinner(640,293,SDL_GetTicks64());c.centered(title,363,32,c.p.text);c.centered(sub,419,23,c.p.muted);
        c.rect(40,650,1200,1,c.p.line);c.text("StockiestThree9",48,676,18,c.p.muted,300);c.present();
    }
    void screenshot(const std::string& path){canvas.capturePath=path;render(rows());}
    void busy(const std::string& title,const std::string& sub,const std::function<void()>& action){
        // Only the main thread draws or handles applet events. Work completes before another action or exit.
        struct Job {std::function<void()> action;std::atomic<bool> done{false};std::exception_ptr failure;};
        Job job;job.action=action;loading(title,sub);
        SDL_Thread* worker=SDL_CreateThreadWithStackSize([](void* raw)->int{auto& j=*static_cast<Job*>(raw);try{j.action();}catch(...){j.failure=std::current_exception();}j.done.store(true,std::memory_order_release);return 0;},"zse-load",512*1024,&job);
        if(!worker)throw Error("Could not start the loading task. Please reopen the editor.");
        std::unique_ptr<SDL_Thread,void(*)(SDL_Thread*)> joined(worker,[](SDL_Thread* thread){SDL_WaitThread(thread,nullptr);});
        while(!job.done.load(std::memory_order_acquire)){if(platform.poll().quit)running=false;loading(title,sub);SDL_Delay(16);}
        joined.reset();if(platform.poll().quit)running=false;dirty=true;rebuild=true;
        if(job.failure)std::rethrow_exception(job.failure);
    }
    void render(const std::vector<Row>& entries){
        auto& c=canvas;c.start();auto& f=current();
        c.logo(42,33,48);c.text(f.title,110,31,32,c.p.text,900);
        c.text(f.subtitle.empty()?"Zelda Save Editor":f.subtitle,110,76,19,c.p.muted,save?925:1110);
        if(save&&save->dirty())c.text("Unsaved",1080,50,21,c.p.accent,150,true);
        if(save)c.text(slot.profileLabel(),1064,79,18,c.p.accent,166,true);
        c.rect(40,112,1200,1,c.p.line);
        bool cards=f.view==View::Games||f.view==View::Profiles;
        if(f.view==View::Games){
            c.text("Select a game",48,149,24,c.p.muted,600);
            for(size_t i=0;i<entries.size();++i){int y=204+int(i)*140;c.rect(52,y,1176,112,c.p.panel);if(int(i)==f.selected)c.border(48,y-4,1184,120,c.p.accent,4);c.logo(72,y+16,80);
                c.text(entries[i].title,180,y+12,29,c.p.text,840);c.text(entries[i].subtitle,181,y+59,21,c.p.muted,800);c.text("Open",1075,y+37,24,c.p.accent,125,true);
            }
            c.text("X  Appearance and sound settings",52,594,20,c.p.muted,1150);
        }else if(f.view==View::Slots){
            auto visible=filteredSlots();const char* names[]={"Normal","Profile 2","Master Mode","Other"};int width=1168/tabCount();
            for(int t=0;t<tabCount();++t){int x=56+t*width;int count=int(std::count_if(slots.begin(),slots.end(),[t](auto& s){return inTab(s,SaveTab(t));}));bool active=t==int(f.saveTab);
                c.text(std::string(isTotk()?"Your saves":names[t])+"  ("+std::to_string(count)+")",x+18,133,24,active?c.p.accent:c.p.muted,width-36);c.rect(x,177,width-12,active?4:1,active?c.p.accent:c.p.line);
            }
            const char* note=isTotk()?"Tears of the Kingdom · Your manual save and autosaves, newest first.":f.saveTab==SaveTab::Normal?"Profile 1 · The normal-mode saves used by this Switch.":f.saveTab==SaveTab::Profile2?"Switch 2 save location · Not playable here as Profile 2. Use Y to swap this set into Profile 1.":f.saveTab==SaveTab::Master?"Master Mode · Managed separately. These saves stay in their current locations.":"Additional save locations. Their original folders are preserved when editing.";
            c.text(note,56,201,20,f.saveTab==SaveTab::Profile2?c.p.accent:c.p.muted,1168);
            if(entries.empty()){c.text("No saves in this tab",60,298,32,c.p.text,1000);c.wrapped(isTotk()?"Make a save in Tears of the Kingdom, close the game, then reopen the editor.":"Use L / R to choose another save category.",60,357,900,24,c.p.muted);}
            else{
                const auto& selected=*visible[f.selected];
                c.rect(56,251,320,180,c.p.panel);
                c.imageBytes("slot"+std::to_string(profile.low)+"-"+selected.folder,selected.image,88,269,256,144);
                c.wrapped(location(selected),56,450,322,27,c.p.text,2);
                c.text(selected.profileLabel(),56,537,20,c.p.accent,322);
                c.wrapped(selected.date,56,575,322,20,c.p.muted,2);
                c.rect(408,250,1,376,c.p.line);
                for(int i=f.scroll;i<std::min(int(entries.size()),f.scroll+5);++i){int y=251+(i-f.scroll)*72;const auto& s=*visible[i];bool focus=i==f.selected;
                    if(focus){c.rect(434,y,776,68,c.p.focus);c.border(430,y-4,784,76,c.p.accent,3);}else c.rect(446,y+70,750,1,c.p.line);
                    c.imageBytes("slot"+std::to_string(profile.low)+"-"+s.folder,s.image,447,y+9,96,54);
                    c.text(s.label,560,y+7,24,c.p.text,467);
                    c.text(s.profileLabel(),1040,y+12,18,focus?c.p.accent:c.p.muted,152,true);
                    c.text(location(s),560,y+42,19,c.p.muted,300);
                    c.text(s.date,840,y+43,17,c.p.muted,351,true);
                }
                c.text(std::to_string(f.selected+1)+" / "+std::to_string(entries.size())+" saves",930,619,18,c.p.muted,277,true);
            }
        }else if(cards){
            int columns=f.view==View::Profiles?4:3;int rowsCount=f.view==View::Profiles?1:2;int pageSize=columns*rowsCount;int page=f.selected/pageSize;
            int width=f.view==View::Profiles?270:370;int height=f.view==View::Profiles?334:218;
            int startX=f.view==View::Profiles?64:61;int gap=f.view==View::Profiles?23:25;
            for(int i=page*pageSize;i<std::min(int(entries.size()),(page+1)*pageSize);++i){int local=i%pageSize;int x=startX+(local%columns)*(width+gap),y=151+(local/columns)*(height+22);
                c.rect(x,y,width,height,c.p.panel);if(i==f.selected)c.border(x-4,y-4,width+8,height+8,c.p.accent,4);
                if(f.view==View::Profiles){c.imageBytes("profile"+std::to_string(i),profiles[i].image,x+42,y+30,186,186);c.text(entries[i].title,x+18,y+248,25,c.p.text,width-36);}
                else{c.imageBytes("slot"+std::to_string(profile.low)+"-"+std::to_string(i),slots[i].image,x+4,y+4,width-8,147);c.text(entries[i].title,x+14,y+158,23,c.p.text,width-28);c.text(entries[i].subtitle,x+14,y+188,17,c.p.muted,width-28);}
            }
            if(entries.empty()){c.text("No saves found for this user.",68,248,30,c.p.text,1000);c.text("Create an in-game save, close BOTW, then try again.",68,310,23,c.p.muted,1080);}
            else c.text("Page "+std::to_string(page+1)+" / "+std::to_string((entries.size()+pageSize-1)/pageSize),900,616,18,c.p.muted,314,true);
        }else{
            // A fixed row grid keeps type size and value alignment constant while navigating.
            c.text(f.view==View::Inventory?sectionName(f.group):f.view==View::Settings?"Preferences":f.view==View::Swap?"Game profiles":gameName(selectedGame),48,155,25,c.p.text,246);
            c.wrapped(f.view==View::Settings?"Make the editor feel at home.":f.view==View::Swap?"Switch between two normal adventures. Both sets of saves are retained.":f.view==View::Inventory?"L / R\nChange section":f.filter.empty()?"Choose a value to edit. Changes stay here until you save.":"Search: "+f.filter,48,209,240,21,c.p.muted,6);
            if(save&&f.view!=View::Settings){c.text(std::to_string(save->changedWords())+" changes",48,548,22,c.p.accent,238);c.text("+  Save changes",48,584,21,c.p.muted,238);}
            c.rect(310,151,1,475,c.p.line);
            const int visible=6;int end=std::min(int(entries.size()),f.scroll+visible);
            for(int i=f.scroll;i<end;++i){int y=149+(i-f.scroll)*76;auto& row=entries[i];bool selected=i==f.selected;
                if(selected){c.rect(338,y,871,72,c.p.focus);c.border(334,y-4,879,80,c.p.accent,3);}
                int labelWidth=row.value.empty()?810:590;
                c.text(row.title,356,y+8,25,row.danger?c.p.danger:c.p.text,labelWidth);
                c.text(row.subtitle,356,y+43,17,c.p.muted,810);
                if(!row.value.empty())c.text(row.value,970,y+11,25,c.p.accent,221,true);
                if(!selected)c.rect(350,y+74,851,1,c.p.line);
            }
            if(entries.empty())c.text("No matching entries",360,200,28,c.p.muted,800);
            if(entries.size()>visible){int h=std::max(12,456*visible/int(entries.size()));int y=149+int(double(f.scroll)*(456-h)/std::max(1,int(entries.size())-visible));c.rect(1226,149,3,456,c.p.line);c.rect(1224,y,7,h,c.p.accent);}
            c.text(entries.empty()?"":std::to_string(f.selected+1)+" / "+std::to_string(entries.size()),940,618,17,c.p.muted,270,true);
        }
        footer(bool(dialog));
        if(!toast.empty()&&SDL_GetTicks64()<toastUntil&&!dialog){
            if(save){c.rect(32,378,270,152,c.p.panel);c.border(32,378,270,152,c.p.line,1);c.wrapped(toast,48,393,238,20,c.p.text,4);}
            else{c.rect(290,561,700,60,c.p.panel);c.border(290,561,700,60,c.p.line,1);c.text(toast,316,576,24,c.p.text,650);}
        }
        if(dialog){auto d=*dialog;c.rect(0,0,1280,720,{0,0,0,145});c.rect(230,166,820,400,c.p.panel);c.text(d.title,274,199,32,c.p.text,730);c.wrapped(d.message,274,263,730,23,c.p.text,6);
            c.rect(230,483,820,1,c.p.line);if(d.confirm){c.rect(640,484,1,82,c.p.line);c.text("Cancel",276,507,25,c.p.text,320);c.text(d.accept,687,507,25,c.p.accent,319);c.border(d.selected?644:234,488,402,74,c.p.accent,3);}else{c.text("OK",622,507,25,c.p.accent,100);c.border(234,488,812,74,c.p.accent,3);}}
        c.present();dirty=false;
    }
    void handle(const Input& input,const std::vector<Row>& entries){
        if(input.quit){running=false;return;}
        if(dialog){
            if(input.down&(Left|Right)){dialog->selected=1-dialog->selected;platform.cue(Cue::Focus);dirty=true;}
            if(input.down&B){dialog.reset();platform.cue(Cue::Back);dirty=true;}
            else if(input.down&A || (input.touchY>=483&&input.touchY<=566&&input.touchX>=230&&input.touchX<=1050)){
                bool accept=!dialog->confirm||dialog->selected==1;if(input.touchX>=0&&dialog->confirm)accept=input.touchX>=640;
                auto action=dialog->action;dialog.reset();platform.cue(Cue::Confirm);if(accept)tryAction(action);dirty=true;
            }return;
        }
        if(input.down&X){if(current().view!=View::Settings)push(View::Settings,"Settings");else back();return;}
        if(input.down&B){back();return;}
        if(current().view==View::Slots){
            if(input.down&(L|R)){changeSaveTab((int(current().saveTab)+((input.down&R)?1:tabCount()-1))%tabCount());return;}
            if(input.down&Y){if(!isTotk()&&(current().saveTab==SaveTab::Normal||current().saveTab==SaveTab::Profile2))tryAction([this]{manageProfiles();});return;}
            if(input.touchY>=124&&input.touchY<=183&&input.touchX>=56&&input.touchX<1224){changeSaveTab(std::min(tabCount()-1,(input.touchX-56)/(1168/tabCount())));return;}
        }
        if(input.down&Plus){if(save)tryAction([this]{review();});else running=false;return;}
        if(save&&current().view!=View::Settings){
            auto historyChanged=[this]{
                // Item insertion/removal can change the meaning of an ordinal.
                // Discard child screens with captured field callbacks after undo.
                if(isTotk())for(size_t i=0;i<stack.size();++i)if(stack[i].view==View::Inventory){stack.resize(i+1);break;}
                rebuild=true;
            };
            if(input.down&ZL){auto label=save->undoLabel();if(save->undo()){notice("Undid "+label);historyChanged();}else notice("Nothing to undo");return;}
            if(input.down&ZR){if(save->redo()){notice("Edit restored");historyChanged();}else notice("Nothing to redo");return;}
        }
        if(input.down&Y&&(current().view==View::Fields||current().view==View::Inventory||current().view==View::Picker)){tryAction([this]{search();});return;}
        if(current().view==View::Inventory&&(input.down&(L|R))){int direction=(input.down&R)?1:-1;current().group=ItemGroup((int(current().group)+direction+9)%9);current().selected=0;current().scroll=0;platform.cue(Cue::Focus);dirty=true;rebuild=true;return;}
        auto& f=current();int movement=0;int cols=f.view==View::Profiles?4:1;
        if(input.down&Up)movement-=cols;if(input.down&Down)movement+=cols;
        int visible=f.view==View::Slots?5:6;
        if(input.down&Left)movement-=cols>1?1:visible;if(input.down&Right)movement+=cols>1?1:visible;
        if(movement&&!entries.empty()){int next=std::clamp(f.selected+movement,0,int(entries.size())-1);if(next!=f.selected){f.selected=next;platform.cue(Cue::Focus);dirty=true;}}
        if(input.touchX>=0){int chosen=-1;
            if(f.view==View::Games){if(input.touchX>=48&&input.touchX<=1232)for(int i=0;i<int(entries.size());++i)if(input.touchY>=200+i*140&&input.touchY<320+i*140)chosen=i;}
            else if(f.view==View::Profiles){int localX=input.touchX-64,localY=input.touchY-151;if(localX>=0&&localY>=0&&localX<4*293&&localY<334)chosen=(f.selected/4)*4+localX/293;}
            else if(f.view==View::Slots){if(input.touchX>=430&&input.touchX<=1214&&input.touchY>=251&&input.touchY<611)chosen=f.scroll+(input.touchY-251)/72;}
            else if(input.touchX>=334&&input.touchX<=1213&&input.touchY>=149&&input.touchY<605)chosen=f.scroll+(input.touchY-149)/76;
            if(chosen>=0&&chosen<int(entries.size())){f.selected=chosen;platform.cue(Cue::Confirm);tryAction(entries[chosen].action);return;}
        }
        if(f.selected<f.scroll)f.scroll=f.selected;if(f.selected>=f.scroll+visible)f.scroll=f.selected-visible+1;
        if(input.down&A&&!entries.empty()&&f.selected<int(entries.size())){auto action=entries[f.selected].action;platform.cue(Cue::Confirm);tryAction(action);}
    }
    int run(int argc,char** argv){
        busy("Getting things ready","Loading names and definitions from your installed game",[&]{platform.loadMetadata(metadata);});
#ifndef __SWITCH__
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
            auto add=rows().front().action;add();ensure(current().view==View::Item,"New item does not open for customization");screenshot("preview/totk-new-item.png");
            handle(Input{ZL},rows());ensure(current().view==View::Inventory&&save->bytes()==freeSlot,"Undo retained stale item callbacks");
            handle(Input{ZL},rows());ensure(save->bytes()==before,"UI undo failed to restore fixture");
            current().group=ItemGroup::Food;current().selected=0;handle(Input{A},rows());screenshot("preview/totk-meal.png");
            for(auto row:rows())if(row.title=="Meal effect"){row.action();break;}
            ensure(current().view==View::Choices,"Meal effect picker missing");screenshot("preview/totk-meal-effects.png");back();back();
            current().group=ItemGroup::Materials;current().selected=int(rows().size())-1;current().scroll=std::max(0,current().selected-5);screenshot("preview/totk-fill-stacks.png");
            ensure(save->bytes()==before,"Browsing new screens edited data");
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
#else
        (void)argc;(void)argv;
#endif
        std::vector<Row> entries;bool lastDark=platform.dark();uint64_t themeCheck=0;
        while(running){if(rebuild){entries=rows();rebuild=false;}if(!entries.empty())current().selected=std::clamp(current().selected,0,int(entries.size())-1);else current().selected=0;
            if(current().scroll>current().selected)current().scroll=current().selected;
            if(SDL_GetTicks64()>=themeCheck){themeCheck=SDL_GetTicks64()+500;bool dark=platform.dark();if(dark!=lastDark){lastDark=dark;dirty=true;}}
            if(dirty)render(entries);auto input=platform.poll();handle(input,entries);
            if(!toast.empty()&&SDL_GetTicks64()>=toastUntil){toast.clear();dirty=true;}SDL_Delay(12);
        }return 0;
    }
};
}
int runApp(int argc,char** argv){
    SDL_SetMainReady();
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS)!=0)throw Error(SDL_GetError());
    if(TTF_Init()!=0){SDL_Quit();throw Error(TTF_GetError());}IMG_Init(IMG_INIT_JPG|IMG_INIT_PNG);
    int result=0;try{{App app;result=app.run(argc,argv);}}catch(...){IMG_Quit();TTF_Quit();SDL_Quit();throw;}
    IMG_Quit();TTF_Quit();SDL_Quit();return result;
}
}
