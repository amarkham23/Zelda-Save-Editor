// Copyright (c) 2026 StockiestThree9
#include "save.hpp"
#include "resources.hpp"
#include "botw.hpp"
#include <iostream>
#include <cmath>
#include <cstring>
#include <limits>
#include <fstream>
#include <algorithm>
using namespace zse;
static int passed=0;
static void expect(bool ok,const char* name){if(!ok)throw Error(std::string("FAILED: ")+name);++passed;}
template<class F>static void rejects(F fn,const char* name){try{fn();}catch(const Error&){++passed;return;}throw Error(std::string("DID NOT REJECT: ")+name);}
static void append(Bytes& b,uint32_t v,bool be=false){for(int i=0;i<4;++i)b.push_back(uint8_t(v>>(be?(3-i)*8:i*8)));}
static Bytes fixture(bool be=false){
    Bytes b;append(b,0x471e,be);append(b,0xffffffff,be);append(b,1,be);
    for(auto name:{"Integer","Real","Bool","Text"})for(int j=0;j<(std::string(name)=="Text"?8:1);++j){append(b,hashName(name),be);append(b,0,be);}
    append(b,0xffffffff,be);return b;
}
static void synthetic(){
    expect(hashName("123456789")==0xcbf43926,"CRC32 reference");
    for(bool be:{false,true}){
        auto b=fixture(be);Save s(b);expect(s.bigEndian()==be,"endianness");expect(s.bytes()==b,"no-op exact bytes");
        s.edit("values",[&]{s.integer(hashName("Integer"),0,-123);s.real(hashName("Real"),0,12.75f);s.word(hashName("Bool"),0,1);s.string(hashName("Text"),32,0,"StockiestThree9");});
        expect(s.integer(hashName("Integer"))==-123,"signed integer");expect(s.real(hashName("Real"))==12.75f,"float");expect(s.string(hashName("Text"),32)=="StockiestThree9","raw text");
        Save round(s.bytes());expect(round.bytes()==s.bytes(),"edited round trip");expect(s.undo()&&s.bytes()==b,"undo exact bytes");expect(s.redo()&&s.bytes()==round.bytes(),"redo exact bytes");
        auto snapshot=s.bytes();rejects([&]{s.edit("invalid",[&]{s.integer(hashName("Integer"),0,999);s.integer(hashName("Integer"),1,2);});},"array overflow");expect(s.bytes()==snapshot,"failed transaction rollback");
        rejects([&]{s.edit("nan",[&]{s.real(hashName("Real"),0,std::numeric_limits<float>::quiet_NaN());});},"NaN");
        rejects([&]{s.edit("long",[&]{s.string(hashName("Text"),32,0,std::string(32,'x'));});},"string overflow");
        rejects([&]{s.word(hashName("Bool"),0,0);},"unjournaled edits");
        expect(s.original()==b,"original immutable until commit");s.accepted();expect(!s.dirty()&&!s.undo(),"commit history reset");
    }
    for(size_t n:{size_t(0),size_t(3),size_t(15),size_t(20)})rejects([&]{Save s{Bytes(n)};},"short file");
    auto b=fixture();b.pop_back();rejects([&]{Save s(b);},"truncated file");b=fixture();b.back()=0;rejects([&]{Save s(b);},"missing sentinel");
    b=fixture();b[0]=0;rejects([&]{Save s(b);},"unknown version");
    Bytes yaz={'Y','a','z','0',0,0,0,3,0,0,0,0,0,0,0,0,0xe0,'a','b','c'};expect(decompress(yaz)==Bytes({'a','b','c'}),"Yaz0 literals");
    yaz[16]=0;rejects([&]{decompress(yaz);},"Yaz0 bad backref");
    rejects([&]{unpackSarc(Bytes(32));},"invalid SARC");rejects([&]{readByml(Bytes(32));},"invalid BYML");rejects([&]{readMsbt(Bytes(32));},"invalid MSBT");
}
static void qualityRegressions(){
    for(bool be:{false,true}){
        auto original=fixture(be);Save s(original);auto h=hashName("Integer");
        auto verify=[&]{size_t count=0;for(auto& field:s.fields())for(auto at:field.second)count+=std::memcmp(s.bytes().data()+at,s.original().data()+at,4)!=0;expect(s.changedWords()==count&&s.dirty()==(count!=0),"cached change count agrees with bytes");};
        s.edit("one",[&]{s.word(h,0,1);});verify();s.undo();verify();s.redo();verify();
        s.edit("original value",[&]{s.word(h,0,0);});verify();s.undo();verify();
        s.edit("new branch",[&]{s.word(h,0,2);});verify();expect(!s.redo(),"new edit clears redo history");
        auto before=s.bytes();rejects([&]{s.edit("rollback",[&]{s.word(h,0,9);throw Error("test");});},"failed edit");expect(s.bytes()==before,"rollback preserves bytes");verify();
        for(auto action:{0,1,2})rejects([&]{s.edit("nested history",[&]{s.word(h,0,20);if(action==0)s.undo();else if(action==1)s.redo();else s.accepted();});},"history mutation during transaction");
        expect(s.bytes()==before,"history refusal rolls back action");verify();
        s.edit("text",[&]{s.string(hashName("Text"),32,0,"hello");});verify();s.undo();verify();s.redo();verify();
        for(unsigned i=3;i<110;++i)s.edit("bounded",[&]{s.word(h,0,i);});
        expect(s.undoCount()==100,"bounded history");while(s.undo())verify();while(s.redo())verify();s.accepted();verify();
    }
    Bytes b;append(b,0x471e);append(b,0xffffffff);append(b,1);
    for(auto key:{"WeaponPorchStockNum","BowPorchStockNum","ShieldPorchStockNum","MaxHartValue","CurrentHart"}){append(b,hashName(key));append(b,12);}
    for(unsigned i=0;i<12*16;++i){append(b,hashName("PorchItem"));append(b,0);}
    for(unsigned i=0;i<12;++i){append(b,hashName("PorchItem_EquipFlag"));append(b,0);}
    append(b,0xffffffff);Save s(b);
    s.edit("setup",[&]{for(unsigned i=0;i<9;++i)s.string(hashName("PorchItem"),64,i,"Weapon_Sword_001");s.string(hashName("PorchItem"),64,9,"Armor_001_Head");s.string(hashName("PorchItem"),64,10,"Item_Fruit_A");});s.accepted();auto original=s.bytes();
    Metadata m;Botw game(m);
    EditField capacity;capacity.field={"WeaponPorchStockNum",hashName("WeaponPorchStockNum"),Kind::Int};capacity.label="Weapon slots";capacity.minimum=8;capacity.maximum=20;capacity.whole=true;
    rejects([&]{setValue(s,capacity,"8");},"BOTW capacity cannot hide owned weapons");expect(s.bytes()==original,"capacity refusal unchanged");
    setValue(s,capacity,"9");expect(s.integer(capacity.field.hash)==9,"exact owned capacity accepted");s.undo();
    EditField life;life.field={"CurrentHart",hashName("CurrentHart"),Kind::Int};life.label="Current hearts";life.scale=4;life.minimum=0;life.maximum=30;
    rejects([&]{setValue(s,life,"3.25");},"BOTW current hearts cannot exceed maximum");
    auto all=game.items(s);auto hood=all[9];rejects([&]{game.replaceItem(s,hood,"Armor_001_Upper");},"BOTW cross-part armor replacement");
    auto stale=all.front();stale.id="Weapon_Sword_002";
    rejects([&]{game.replaceItem(s,stale,"Weapon_Sword_003");},"BOTW stale replacement");
    rejects([&]{game.removeItem(s,stale);},"BOTW stale removal");
    rejects([&]{game.equipItem(s,stale);},"BOTW stale equip");
    rejects([&]{game.equipItem(s,all[10]);},"BOTW material equip");
    expect(s.bytes()==original,"refused inventory operations unchanged");
    game.replaceItem(s,hood,"Armor_002_Head");expect(s.string(hashName("PorchItem"),64,9)=="Armor_002_Head","BOTW same-part armor replacement");expect(s.undo()&&s.bytes()==original,"replacement undo exact");
}
int main(int argc,char** argv){try{
    synthetic();
    qualityRegressions();
    if(argc>1){
        auto original=readFile(argv[1]);Save s(original);expect(s.bytes()==original,"real no-op exact bytes");
        uint32_t h=hashName("PorchItem_Value1");if(s.count(h)){auto value=s.integer(h);s.edit("one item",[&]{s.integer(h,0,value==999?998:999);});
        expect(s.changedWords()==1,"real single-field isolation");expect(s.undo()&&s.bytes()==original,"real exact undo");}
        std::cout<<"Save version 0x"<<std::hex<<s.version()<<std::dec<<", "<<s.fields().size()<<" fields, "<<s.bytes().size()<<" bytes\n";
        if(argc>2){Metadata m;m.loadGameData(readFile(argv[2]));std::cout<<"Dictionary: "<<m.fields.size()<<" definitions\n";
            if(argc>3){m.loadMessages(readFile(argv[3]));std::cout<<"Item names: "<<m.names.size()<<"; "<<m.itemName("Weapon_Sword_001")<<"\n";expect(m.names.size()>100,"real localized item names");}
            std::ofstream report("field-report.tsv");size_t known=0;
            for(auto& pair:m.fields){auto& f=pair.second;auto n=s.count(f.hash);if(!n)continue;++known;report<<f.name<<'\t'<<int(f.kind)<<'\t'<<f.width<<'\t'<<n<<'\t';
                if(f.kind==Kind::String)report<<s.string(f.hash,f.width*4);else if(f.kind==Kind::Float||f.kind==Kind::Vec2||f.kind==Kind::Vec3||f.kind==Kind::Vec4)report<<s.real(f.hash);else report<<s.integer(f.hash);report<<'\n';}
            std::cout<<"Matched "<<known<<" fields in this save\n";expect(known>10000,"real schema matching");
            Botw game(m);auto items=game.items(s);expect(!items.empty(),"real inventory parsed");
            for(auto& item:items){auto fields=game.itemFields(s,item);expect(!fields.empty(),"typed item fields");for(auto& f:fields)displayValue(s,f);}
            auto stats=game.stats(s);expect(stats.size()>=10,"player fields available");
            for(auto f:stats)if(f.field.name=="CurrentRupee"){
                setValue(s,f,"4321");expect(s.integer(hashName("CurrentRupee"))==4321,"rupee edit");expect(s.changedWords()==1,"rupee isolation");expect(s.undo()&&s.bytes()==original,"rupee exact undo");
                rejects([&]{setValue(s,f,"12.5");},"integer UI rejects fractions");rejects([&]{setValue(s,f,"12 apples");},"numeric trailing text");
            }
            for(auto f:stats)if(f.field.name=="StaminaMax"){
                setValue(s,f,"1.2");expect(s.real(hashName("StaminaMax"))==1200.0f,"stamina wheels converted to game units");expect(s.real(hashName("StaminaCurrentMax"))>=1200.0f,"stamina capacity follows maximum");expect(displayValue(s,f)=="1.2","stamina wheels formatted");expect(s.undo()&&s.bytes()==original,"stamina exact undo");
                rejects([&]{setValue(s,f,"1000");},"raw stamina units refused in wheels UI");
            }
            auto item=items.front();auto details=game.itemFields(s,item);
            for(auto f:details)if(f.label=="Durability"){
                setValue(s,f,"250");expect(s.integer(hashName("PorchItem_Value1"),item.index)==25000,"durability units");expect(s.changedWords()==1,"durability isolation");s.undo();
                rejects([&]{setValue(s,f,"3.5");},"durability rejects fractions");
            }
            game.equipItem(s,item);auto equipped=game.items(s);size_t equippedCount=0;for(auto& i:equipped)if(i.group==item.group&&s.word(hashName("PorchItem_EquipFlag"),i.index))++equippedCount;
            expect(equippedCount<=1,"one equipped weapon");s.undo();expect(s.bytes()==original,"equip exact undo");
            game.removeItem(s,item);auto removed=game.items(s);expect(removed.size()+1==items.size(),"remove count");
            for(size_t i=0;i<removed.size();++i){expect(removed[i].id==items[i+1].id,"remove preserves order");Save old(original);expect(s.word(hashName("PorchItem_Value1"),removed[i].index)==old.word(hashName("PorchItem_Value1"),items[i+1].index),"remove preserves values");}
            expect(s.undo()&&s.bytes()==original,"remove exact undo");
            for(auto id:{"Item_Fruit_A","Item_Cook_A_01","Weapon_Sword_001"}){
                game.addItem(s,id);auto added=game.items(s);expect(added.size()==items.size()+1,"add count");
                expect(std::count_if(added.begin(),added.end(),[&](auto& i){return i.id==id;})==std::count_if(items.begin(),items.end(),[&](auto& i){return i.id==id;})+1,"added correct actor");
                expect(s.undo()&&s.bytes()==original,"add exact undo");
            }
            auto history=s.bytes();game.resetAmiibo(s);if(s.dirty())expect(s.undo()&&s.bytes()==history,"amiibo undo");
            for(size_t horse=0;horse<6;++horse)for(auto f:game.horseFields(s,horse))displayValue(s,f);
        }
    }
    std::cout<<"PASS: "<<passed<<" checks\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
