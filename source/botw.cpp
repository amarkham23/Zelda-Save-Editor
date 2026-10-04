// Copyright (c) 2026 StockiestThree9
#include "botw.hpp"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <limits>
#include <cstring>

namespace zse {
static bool starts(const std::string& s,const std::string& prefix){return s.compare(0,prefix.size(),prefix)==0;}
static bool floating(Kind kind){return kind==Kind::Float||kind==Kind::Vec2||kind==Kind::Vec3||kind==Kind::Vec4;}
std::string readable(const std::string& name){std::string out;char prev=0;for(char ch:name){if(ch=='_'){out+=' ';prev=0;continue;}if(std::isupper(static_cast<unsigned char>(ch))&&(std::islower(static_cast<unsigned char>(prev))||std::isdigit(static_cast<unsigned char>(prev))))out+=' ';out+=ch;prev=ch;}return out;}
std::string displayValue(const Save& s,const EditField& e){
    if(!e.choices.empty()){auto value=s.word(e.field.hash,e.index);for(auto& choice:e.choices)if(choice.first==value)return choice.second;return "Unrecognized bonus";}
    auto k=e.field.kind;
    if(k==Kind::String)return s.string(e.field.hash,e.field.width*4,e.index);
    if(k==Kind::Bool)return s.word(e.field.hash,e.index)?"On":"Off";
    double value=floating(k)?double(s.real(e.field.hash,e.index)):k==Kind::UInt?double(s.word(e.field.hash,e.index)):double(s.integer(e.field.hash,e.index));
    value/=e.scale;
    std::ostringstream out;if(e.whole)out<<std::fixed<<std::setprecision(0)<<std::floor(value);else out<<std::setprecision(7)<<value;return out.str();
}
void setValue(Save& s,const EditField& e,const std::string& text){
    if(!e.choices.empty()){for(auto& choice:e.choices)if(text==std::to_string(choice.first)){s.edit(e.label,[&]{
        s.word(e.field.hash,e.index,choice.first);
        if(s.game()==Game::Totk&&e.field.name.find(".Content.Effect.Type")!=std::string::npos){
            auto valueName=e.field.name.substr(0,e.field.name.size()-4)+"Value";auto hash=s.fieldHash(valueName);
            if(s.count(hash)>e.index&&choice.first!=s.fieldHash("None")&&s.integer(hash,e.index)<0)s.integer(hash,e.index,1);
        }
    });return;}throw Error("Choose one of the listed options.");}
    if(e.field.kind==Kind::String){s.edit(e.label,[&]{s.string(e.field.hash,e.field.width*4,e.index,text);});return;}
    if(e.field.kind==Kind::Bool){s.edit(e.label,[&]{s.word(e.field.hash,e.index,text=="1"?1:0);});return;}
    size_t end=0;double value=0;try{value=std::stod(text,&end);}catch(...){throw Error("Enter a number.");}
    if(end!=text.size()||!std::isfinite(value))throw Error("Enter a finite number without other characters.");
    if(e.whole&&value!=std::floor(value))throw Error("Enter a whole number.");
    if(value<e.minimum||value>e.maximum)throw Error("Value must be between "+std::to_string(int64_t(e.minimum))+" and "+std::to_string(int64_t(e.maximum))+".");
    value*=e.scale;
    if(e.field.name=="PlayerStatus.Life"&&s.count("PlayerStatus.MaxLife")&&value>s.integer(s.fieldHash("PlayerStatus.MaxLife")))throw Error("Current hearts cannot exceed maximum hearts.");
    bool isFloat=floating(e.field.kind);
    if(!isFloat&&(value!=std::floor(value)||value<(e.field.kind==Kind::UInt?0.0:-2147483648.0)||value>(e.field.kind==Kind::UInt?4294967295.0:2147483647.0)))throw Error("That value does not fit this field.");
    s.edit(e.label,[&]{
        if(e.field.name=="PlayerStatus.MaxLife"&&s.count("PlayerStatus.Life")&&s.integer(s.fieldHash("PlayerStatus.Life"))>value)s.integer(s.fieldHash("PlayerStatus.Life"),0,int32_t(value));
        if(e.field.name=="StaminaMax"&&e.scale==1000&&s.count("StaminaCurrentMax")){
            float bonus=std::max(0.0f,s.real(hashName("StaminaCurrentMax"))-s.real(e.field.hash));
            s.real(hashName("StaminaCurrentMax"),0,std::min(5000.0f,float(value)+bonus));
        }
        if(e.field.name=="MaxHartValue"&&e.scale==4&&s.count("CurrentHart")&&s.integer(hashName("CurrentHart"))>value)s.integer(hashName("CurrentHart"),0,int32_t(value));
        if(isFloat)s.real(e.field.hash,e.index,float(value));else if(e.field.kind==Kind::UInt)s.word(e.field.hash,e.index,uint32_t(value));else s.integer(e.field.hash,e.index,int32_t(value));
    });
}
ItemGroup itemGroup(const std::string& id){
    if(starts(id,"Weapon_Sword_")||starts(id,"Weapon_Lsword_")||starts(id,"Weapon_Spear_"))return ItemGroup::Weapons;
    if(starts(id,"Weapon_Bow_"))return ItemGroup::Bows;
    if(starts(id,"Weapon_Shield_"))return ItemGroup::Shields;
    if(starts(id,"Armor_"))return ItemGroup::Armor;
    if(starts(id,"Item_Cook_"))return ItemGroup::Food;
    if(id=="NormalArrow"||id=="FireArrow"||id=="IceArrow"||id=="ElectricArrow"||id=="BombArrow_A"||id=="AncientArrow")return ItemGroup::Arrows;
    if(starts(id,"Item_")||starts(id,"Animal_Insect_"))return ItemGroup::Materials;
    if(starts(id,"Obj_")||starts(id,"GameRomHorseReins_")||starts(id,"GameRomHorseSaddle_"))return ItemGroup::KeyItems;
    return ItemGroup::Unknown;
}
std::string groupName(ItemGroup group){const char* labels[]={"Weapons","Bows","Arrows","Shields","Armor","Materials","Food","Key items","Other"};return labels[int(group)];}
static std::string modifierPrefix(ItemGroup group){if(group==ItemGroup::Weapons)return "PorchSword";if(group==ItemGroup::Bows)return "PorchBow";if(group==ItemGroup::Shields)return "PorchShield";return "";}
EditField Botw::field(const std::string& name,const std::string& label,size_t index) const {
    auto it=metadata.fields.find(hashName(name));if(it==metadata.fields.end())throw Error("Missing game definition: "+name);
    EditField e;e.field=it->second;e.index=index;e.label=label;return e;
}
std::vector<EditField> Botw::stats(const Save& s) const {
    struct Stat {const char* name;const char* label;double scale,min,max;bool whole;};
    const Stat stats[]={
        {"CurrentRupee","Rupees",1,0,999999,true},{"CurrentMamo","Mon",1,0,999999,true},
        {"MaxHartValue","Maximum hearts",4,3,30,true},{"CurrentHart","Current hearts",4,0,30,false},
        {"StaminaMax","Maximum stamina wheels",1000,1,3,false},{"StaminaCurrentMax","Stamina wheels with bonus",1000,0,5,false},
        {"WeaponPorchStockNum","Weapon slots",1,8,20,true},{"BowPorchStockNum","Bow slots",1,5,14,true},{"ShieldPorchStockNum","Shield slots",1,4,20,true},
        {"IsGet_Obj_Motorcycle","Master Cycle Zero",1,0,1,true},
        {"IsGet_Obj_HeroSoul_Zora","Mipha's Grace",1,0,1,true},{"IsGet_Obj_HeroSoul_Goron","Daruk's Protection",1,0,1,true},
        {"IsGet_Obj_HeroSoul_Rito","Revali's Gale",1,0,1,true},{"IsGet_Obj_HeroSoul_Gerudo","Urbosa's Fury",1,0,1,true}
    };
    std::vector<EditField> out;
    for(auto& st:stats)if(s.count(st.name)&&metadata.fields.count(hashName(st.name))){auto e=field(st.name,st.label);e.scale=st.scale;e.minimum=st.min;e.maximum=st.max;e.whole=st.whole;
        if(e.field.name=="StaminaMax"){e.description="Your permanent stamina capacity";e.unit="wheels";}
        if(e.field.name=="StaminaCurrentMax"){e.description="Includes any temporary bonus stamina";e.unit="wheels";}
        if(e.field.name=="CurrentHart"||e.field.name=="MaxHartValue")e.unit="hearts";
        out.push_back(e);}
    return out;
}
std::vector<Item> Botw::items(const Save& s) const {
    std::vector<Item> out;std::array<size_t,9> ordinals{};
    for(size_t i=0;i<s.count("PorchItem")/16;++i){auto id=s.string(hashName("PorchItem"),64,i);if(id.empty())continue;auto group=itemGroup(id);out.push_back({id,i,ordinals[size_t(group)]++,group});}
    return out;
}
std::vector<EditField> Botw::itemFields(const Save& s,const Item& item) const {
    std::vector<EditField> out;auto prefix=modifierPrefix(item.group);
    auto e=field("PorchItem_Value1",prefix.empty()?item.group==ItemGroup::Armor?"Dye color":"Quantity":"Durability",item.index);e.whole=true;e.minimum=0;e.maximum=prefix.empty()?999:1000000;
    if(!prefix.empty()){e.scale=100;e.description="How much use remains before it breaks";e.unit="durability";}out.push_back(e);
    if(!prefix.empty()||item.group==ItemGroup::Armor)out.push_back(field("PorchItem_EquipFlag","Equipped",item.index));
    if(!prefix.empty()){
        auto flags=field(prefix+"_FlagSp","Modifier flags",item.ordinal);flags.field.kind=Kind::UInt;flags.maximum=4294967295.0;flags.minimum=0;flags.whole=true;flags.description="Attack 1 · Durability 2 · Critical 4 · Gold tier +2147483648";out.push_back(flags);
        auto value=field(prefix+"_ValueSp","Modifier strength",item.ordinal);value.whole=true;out.push_back(value);
    }
    if(item.group==ItemGroup::Food){
        const char* keys[]={"CookEffect0","CookEffect0","CookEffect1","StaminaRecover","StaminaRecover"};
        const char* names[]={"Effect type","Effect strength","Sell value","Hearts restored","Effect duration"};
        const size_t component[]={0,1,0,0,1};
        for(size_t i=0;i<5;++i){auto f=field(keys[i],names[i],item.ordinal*2+component[i]);f.whole=true;if(i==2)f.unit="rupees";if(i==3){f.scale=4;f.whole=false;f.unit="hearts";}if(i==4)f.unit="seconds";out.push_back(f);}
    }
    (void)s;return out;
}
std::vector<EditField> Botw::horseFields(const Save& s,size_t horse) const {
    std::vector<EditField> out;
    for(auto key:{"Horse_UserName","Horse_ActorName","Horse_Familiarity","Horse_ManeName","Horse_ReinsName","Horse_SaddleName","Horse_FootType","Horse_CollarType","Horse_RideTimeSec"}){
        auto it=metadata.fields.find(hashName(key));if(it==metadata.fields.end())continue;
        size_t count=s.count(it->first)/it->second.width;if(horse>=count)continue;auto f=field(key,readable(std::string(key).substr(6)),horse);
        if(f.field.name=="Horse_Familiarity"){f.label="Bond";f.minimum=0;f.maximum=100;f.whole=true;f.unit="%";}
        out.push_back(f);
    }
    return out;
}
std::vector<EditField> Botw::matching(const Save& s,const std::function<bool(const Field&)>& predicate) const {
    std::vector<EditField> out;
    for(auto& pair:metadata.fields){auto& f=pair.second;if(s.count(f.hash)&&predicate(f)){auto e=field(f.name,readable(f.name));e.description=f.name;out.push_back(e);}}
    std::sort(out.begin(),out.end(),[](auto& a,auto& b){return a.field.name<b.field.name;});return out;
}
std::vector<EditField> Botw::elements(const Save& s,const Field& f) const {
    std::vector<EditField> out;size_t n=s.count(f.hash)/(f.kind==Kind::String?f.width:1);
    for(size_t i=0;i<n;++i){auto e=field(f.name,"Entry "+std::to_string(i+1),i);if(f.kind==Kind::Vec2||f.kind==Kind::Vec3||f.kind==Kind::Vec4){static const char* axes[]={"X","Y","Z","W"};e.label=(n>f.width?"Entry "+std::to_string(i/f.width+1)+" · ":"")+axes[i%f.width];}out.push_back(e);}
    return out;
}
std::vector<Botw::Entry> Botw::inventory(const Save& s) const {
    std::vector<Entry> out;
    for(auto& item:items(s)){
        Entry e;e.id=item.id;e.value=s.word(hashName("PorchItem_Value1"),item.index);e.equipped=s.word(hashName("PorchItem_EquipFlag"),item.index);
        auto prefix=modifierPrefix(item.group);if(!prefix.empty()){e.flags=s.word(hashName(prefix+"_FlagSp"),item.ordinal);e.modifier=s.word(hashName(prefix+"_ValueSp"),item.ordinal);}
        if(item.group==ItemGroup::Food){const char* keys[]={"CookEffect0","CookEffect1","StaminaRecover"};for(int k=0;k<3;++k)for(int v=0;v<2;++v)e.food[k*2+v]=s.word(hashName(keys[k]),item.ordinal*2+v);
            for(int k=0;k<5;++k)e.ingredients[k]=s.string(hashName("CookMaterialName"+std::to_string(k)),64,item.ordinal);}
        out.push_back(std::move(e));
    }
    return out;
}
void Botw::inventory(Save& s,const std::vector<Entry>& entries) const {
    size_t capacity=s.count("PorchItem")/16;if(entries.size()>capacity)throw Error("The inventory is full.");
    std::array<size_t,9> counts{};
    for(auto& e:entries){auto g=itemGroup(e.id);size_t n=++counts[size_t(g)];auto prefix=modifierPrefix(g);if(!prefix.empty()&&n>s.count(prefix+"_FlagSp"))throw Error("That equipment section is full.");if(g==ItemGroup::Food&&n>s.count("CookEffect0")/2)throw Error("The food section is full.");}
    // Validate all section limits before touching any value; the caller journals this as one edit.
    for(auto p:std::array<std::pair<ItemGroup,const char*>,3>{{{ItemGroup::Weapons,"WeaponPorchStockNum"},{ItemGroup::Bows,"BowPorchStockNum"},{ItemGroup::Shields,"ShieldPorchStockNum"}}})if(s.count(p.second)&&counts[size_t(p.first)]>size_t(std::max(0,s.integer(hashName(p.second)))))throw Error("Increase the number of "+groupName(p.first)+" slots in Player before adding more.");
    counts.fill(0);
    for(size_t i=0;i<capacity;++i){
        const Entry empty{};auto& e=i<entries.size()?entries[i]:empty;
        s.string(hashName("PorchItem"),64,i,e.id);s.word(hashName("PorchItem_Value1"),i,e.value);s.word(hashName("PorchItem_EquipFlag"),i,e.equipped);
        if(e.id.empty())continue;auto g=itemGroup(e.id);size_t ord=counts[size_t(g)]++;auto prefix=modifierPrefix(g);
        if(!prefix.empty()){s.word(hashName(prefix+"_FlagSp"),ord,e.flags);s.word(hashName(prefix+"_ValueSp"),ord,e.modifier);}
        if(g==ItemGroup::Food){const char* keys[]={"CookEffect0","CookEffect1","StaminaRecover"};for(int k=0;k<3;++k)for(int v=0;v<2;++v)s.word(hashName(keys[k]),ord*2+v,e.food[k*2+v]);for(int k=0;k<5;++k)s.string(hashName("CookMaterialName"+std::to_string(k)),64,ord,e.ingredients[k]);}
    }
}
void Botw::replaceItem(Save& s,const Item& item,const std::string& id) const {
    if(itemGroup(id)!=item.group)throw Error("Choose an item from the same inventory section.");
    s.edit("Replace "+metadata.itemName(item.id),[&]{s.string(hashName("PorchItem"),64,item.index,id);});
}
void Botw::addItem(Save& s,const std::string& id) const {
    auto group=itemGroup(id);if(group==ItemGroup::Unknown)throw Error("That item type cannot be added.");
    auto list=inventory(s);Entry e;e.id=id;e.value=modifierPrefix(group).empty()?1:10000;if(group==ItemGroup::Armor)e.value=0;
    if(group==ItemGroup::Food){float negative=-1;uint32_t bits;std::memcpy(&bits,&negative,4);e.food[0]=bits;}
    auto pos=list.end();for(auto it=list.begin();it!=list.end();++it)if(itemGroup(it->id)==group)pos=it+1;
    list.insert(pos,e);s.edit("Add "+metadata.itemName(id),[&]{inventory(s,list);});
}
void Botw::removeItem(Save& s,const Item& item) const {
    auto list=inventory(s);auto all=items(s);auto it=std::find_if(all.begin(),all.end(),[&](auto& v){return v.index==item.index;});if(it==all.end())throw Error("The item is no longer present.");
    list.erase(list.begin()+std::distance(all.begin(),it));s.edit("Remove "+metadata.itemName(item.id),[&]{inventory(s,list);});
}
void Botw::resetAmiibo(Save& s) const {
    s.edit("Reset today's amiibo scans",[&]{if(s.count("AmiiboLastTouchDate"))s.integer(hashName("AmiiboLastTouchDate"),0,19700101);for(size_t i=0;i<s.count("AmiiboTouchHistory")/16;++i)s.string(hashName("AmiiboTouchHistory"),64,i,"");});
}
void Botw::equipItem(Save& s,const Item& item) const {
    bool equip=s.word(hashName("PorchItem_EquipFlag"),item.index)==0;
    auto part=[](const std::string& id){auto at=id.find_last_of('_');return at==std::string::npos?id:id.substr(at);};
    s.edit(equip?"Equip item":"Unequip item",[&]{
        if(equip)for(auto& other:items(s))if(other.group==item.group&&(item.group!=ItemGroup::Armor||part(other.id)==part(item.id)))s.word(hashName("PorchItem_EquipFlag"),other.index,0);
        s.word(hashName("PorchItem_EquipFlag"),item.index,equip?1:0);
    });
}
}
