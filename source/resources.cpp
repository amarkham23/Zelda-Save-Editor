// Copyright (c) 2026 StockiestThree9
#include "resources.hpp"
#include <algorithm>
#include <cstring>
#include <set>

namespace zse {
namespace {
struct Reader {
    const Bytes& b;
    bool be=false;
    void check(size_t p,size_t n) const { if(p>b.size() || n>b.size()-p)throw Error("Truncated game resource."); }
    uint8_t u8(size_t p) const { check(p,1);return b[p]; }
    uint32_t number(size_t p,size_t n) const { check(p,n);uint32_t v=0;for(size_t i=0;i<n;++i)v|=uint32_t(b[p+i])<<(be?(n-i-1)*8:i*8);return v; }
    uint32_t u32(size_t p) const { return number(p,4); }
    uint32_t u24(size_t p) const { return number(p,3); }
    uint16_t u16(size_t p) const { return uint16_t(number(p,2)); }
    bool magic(size_t p,const char* s,size_t n) const { check(p,n);return std::memcmp(b.data()+p,s,n)==0; }
    std::string str(size_t p,size_t limit=4096) const { check(p,1);size_t end=p;while(end<b.size()&&b[end]&&end-p<limit)++end;if(end==b.size()||end-p==limit)throw Error("Unterminated resource string.");return std::string(b.begin()+p,b.begin()+end); }
    Bytes slice(size_t p,size_t n) const { check(p,n);return Bytes(b.begin()+p,b.begin()+p+n); }
};
void utf8(std::string& out,uint32_t cp) {
    if(cp<0x80)out+=char(cp);
    else if(cp<0x800){out+=char(0xc0|(cp>>6));out+=char(0x80|(cp&63));}
    else if(cp<0x10000){out+=char(0xe0|(cp>>12));out+=char(0x80|((cp>>6)&63));out+=char(0x80|(cp&63));}
    else if(cp<=0x10ffff){out+=char(0xf0|(cp>>18));out+=char(0x80|((cp>>12)&63));out+=char(0x80|((cp>>6)&63));out+=char(0x80|(cp&63));}
}
struct ByReader {
    Reader r;
    std::vector<std::string> keys,strings;
    std::set<size_t> active;
    size_t visited=0;
    std::vector<std::string> table(size_t p) {
        if(!p)return {};
        if(r.u8(p)!=0xc2)throw Error("Invalid BYML string table.");
        size_t n=r.u24(p+1);if(n>200000)throw Error("BYML table is too large.");r.check(p+4,(n+1)*4);
        std::vector<std::string> result;result.reserve(n);
        for(size_t i=0;i<n;++i)result.push_back(r.str(p+r.u32(p+4+i*4)));
        return result;
    }
    ByNode value(uint8_t type,uint32_t raw,size_t depth) {
        if(depth>40 || ++visited>1000000)throw Error("BYML nesting limit exceeded.");
        ByNode node;node.type=type;node.number=raw;
        if(type==0xa0){if(raw>=strings.size())throw Error("Invalid BYML text index.");node.text=strings[raw];}
        else if(type==0xc0 || type==0xc1) {
            size_t p=raw;
            if(!active.insert(p).second)throw Error("Cyclic BYML resource.");
            if(r.u8(p)!=type)throw Error("Unexpected BYML container.");
            size_t n=r.u24(p+1);if(n>200000)throw Error("BYML container is too large.");
            if(type==0xc0) {
                r.check(p+4,n);size_t vals=(p+4+n+3)&~size_t(3);r.check(vals,n*4);
                node.array.reserve(n);for(size_t i=0;i<n;++i)node.array.push_back(value(r.u8(p+4+i),r.u32(vals+i*4),depth+1));
            } else {
                r.check(p+4,n*8);
                for(size_t i=0;i<n;++i){size_t e=p+4+i*8;uint32_t key=r.u24(e);if(key>=keys.size())throw Error("Invalid BYML key index.");node.dict.emplace(keys[key],value(r.u8(e+3),r.u32(e+4),depth+1));}
            }
            active.erase(p);
        } else if(type==0xd4||type==0xd5||type==0xd6){r.check(raw,8);node.wide=r.be?(uint64_t(r.u32(raw))<<32)|r.u32(raw+4):uint64_t(r.u32(raw))|(uint64_t(r.u32(raw+4))<<32);}
        else if(type!=0xd0&&type!=0xd1&&type!=0xd2&&type!=0xd3&&type!=0xff) throw Error("Unsupported BYML value type.");
        return node;
    }
};
}
Bytes decompress(const Bytes& data) {
    Reader r{data,true};
    if(data.size()<4 || !r.magic(0,"Yaz0",4))return data;
    r.check(0,16);size_t n=r.u32(4);
    if(n>128*1024*1024)throw Error("Compressed resource is too large.");
    Bytes out;out.reserve(n);size_t p=16;
    while(out.size()<n) {
        uint8_t flags=r.u8(p++);
        for(int bit=7;bit>=0 && out.size()<n;--bit) {
            if(flags&(1<<bit))out.push_back(r.u8(p++));
            else {
                uint8_t a=r.u8(p++),b=r.u8(p++);size_t distance=((a&15)<<8)+b+1;
                size_t len=a>>4;len=len?len+2:r.u8(p++)+0x12;
                if(distance>out.size() || len>n-out.size())throw Error("Invalid compressed back reference.");
                for(size_t i=0;i<len;++i)out.push_back(out[out.size()-distance]);
            }
        }
    }
    return out;
}
std::map<std::string,Bytes> unpackSarc(const Bytes& input) {
    Bytes data=decompress(input);Reader r{data};
    if(!r.magic(0,"SARC",4))throw Error("Game archive has no SARC header.");
    r.be=r.u8(6)==0xfe;
    if(r.u16(6)!=0xfeff)throw Error("Invalid archive byte order.");
    if(r.u32(8)!=data.size())throw Error("Invalid archive length.");
    size_t p=r.u16(4),base=r.u32(12);
    if(!r.magic(p,"SFAT",4))throw Error("Missing archive index.");
    size_t n=r.u16(p+6),nodes=p+r.u16(p+4);r.check(nodes,n*16);
    size_t sfnt=nodes+n*16;if(!r.magic(sfnt,"SFNT",4))throw Error("Missing archive names.");
    size_t names=sfnt+r.u16(sfnt+4);
    std::map<std::string,Bytes> files;
    for(size_t i=0;i<n;++i) {
        size_t at=nodes+i*16;uint32_t attr=r.u32(at+4);size_t start=r.u32(at+8),end=r.u32(at+12);
        if(end<start)throw Error("Invalid archive entry.");
        if(!(attr>>24))continue;
        files.emplace(r.str(names+(attr&0xffffff)*4),r.slice(base+start,end-start));
    }
    return files;
}
const ByNode* ByNode::get(const std::string& key) const { auto it=dict.find(key);return it==dict.end()?nullptr:&it->second; }
ByNode readByml(const Bytes& bytes) {
    Reader r{bytes};r.check(0,16);
    if(r.magic(0,"BY",2))r.be=true;else if(!r.magic(0,"YB",2))throw Error("Invalid BYML header.");
    if(r.u16(2)<2||r.u16(2)>7)throw Error("Unsupported BYML version.");
    ByReader br{r,{},{},{},0};br.keys=br.table(r.u32(4));br.strings=br.table(r.u32(8));
    auto root=r.u32(12);if(!root)return {};return br.value(r.u8(root),root,0);
}
std::map<std::string,std::string> readMsbt(const Bytes& bytes) {
    Reader r{bytes};if(!r.magic(0,"MsgStdBn",8))throw Error("Invalid message table.");
    r.be=r.u8(8)==0xfe;r.check(0,32);
    uint8_t encoding=r.u8(12);size_t blocks=r.u16(14),p=32,lbl=0,txt=0;
    for(size_t i=0;i<blocks;++i) {
        r.check(p,16);size_t n=r.u32(p+4);r.check(p+16,n);
        if(r.magic(p,"LBL1",4))lbl=p+16;
        if(r.magic(p,"TXT2",4))txt=p+16;
        p=(p+16+n+15)&~size_t(15);
    }
    if(!lbl||!txt)return {};
    size_t n=r.u32(txt);if(n>200000)throw Error("Too many messages.");r.check(txt+4,n*4);
    std::vector<std::string> messages;messages.reserve(n);
    for(size_t i=0;i<n;++i) {
        size_t pos=txt+r.u32(txt+4+i*4);std::string text;
        for(size_t chars=0;chars<32768;++chars) {
            uint32_t ch=encoding==0?r.u8(pos):encoding==1?r.u16(pos):r.u32(pos);pos+=encoding==0?1:encoding==1?2:4;
            if(!ch)break;
            if(ch==0xe){r.check(pos,6);size_t extra=r.u16(pos+4);pos+=6+extra;r.check(pos,0);continue;}
            if(ch==0xf){pos+=4;r.check(pos,0);continue;}
            if(encoding==1 && ch>=0xd800 && ch<=0xdbff){uint32_t lo=r.u16(pos);pos+=2;if(lo<0xdc00||lo>0xdfff)throw Error("Invalid UTF-16 message.");ch=0x10000+((ch-0xd800)<<10)+(lo-0xdc00);}
            if(encoding==0)text+=char(ch);else utf8(text,ch);
        }
        messages.push_back(std::move(text));
    }
    size_t groups=r.u32(lbl);if(groups>100000)throw Error("Too many message groups.");r.check(lbl+4,groups*8);
    std::map<std::string,std::string> result;
    for(size_t g=0;g<groups;++g) {
        size_t num=r.u32(lbl+4+g*8),at=lbl+r.u32(lbl+8+g*8);if(num>n)throw Error("Invalid message group.");
        for(size_t j=0;j<num;++j) {
            size_t len=r.u8(at++);r.check(at,len+4);std::string label(bytes.begin()+at,bytes.begin()+at+len);at+=len;
            size_t index=r.u32(at);at+=4;if(index>=messages.size())throw Error("Invalid message index.");result.emplace(std::move(label),messages[index]);
        }
    }
    return result;
}
void Metadata::loadGameData(const Bytes& bootup) {
    auto pack=unpackSarc(bootup);auto it=pack.find("GameData/gamedata.ssarc");
    if(it==pack.end())throw Error("The installed game has no game-data dictionary.");
    auto bg=unpackSarc(it->second);
    std::map<uint32_t,Field> found;
    for(auto& file:bg) {
        if(file.first.size()<7 || file.first.substr(file.first.size()-7)!=".bgdata")continue;
        auto root=readByml(decompress(file.second));
        for(auto& group:root.dict) {
            std::string type=group.first;Field prototype;prototype.array=type.find("array")!=std::string::npos;
            if(type.find("bool")==0)prototype.kind=Kind::Bool;
            else if(type.find("s32")==0)prototype.kind=Kind::Int;
            else if(type.find("f32")==0)prototype.kind=Kind::Float;
            else if(type.find("vector2f")==0){prototype.kind=Kind::Vec2;prototype.width=2;}
            else if(type.find("vector3f")==0){prototype.kind=Kind::Vec3;prototype.width=3;}
            else if(type.find("vector4f")==0){prototype.kind=Kind::Vec4;prototype.width=4;}
            else if(type.find("string")==0){prototype.kind=Kind::String;prototype.width=type.find("256")!=std::string::npos?64:type.find("64")!=std::string::npos?16:8;}
            else continue;
            for(auto& node:group.second.array) {
                auto name=node.get("DataName"),hash=node.get("HashValue");if(!name)continue;
                Field f=prototype;f.name=name->text;f.hash=hash?hash->number:hashName(f.name);
                // The shipped dictionary includes a dummy entry with a synthetic hash.
                // Do not expose a named editor when its identity cannot be verified.
                if(f.hash!=hashName(f.name))continue;
                found.emplace(f.hash,std::move(f));
            }
        }
    }
    if(found.empty())throw Error("The game-data dictionary is empty.");
    fields=std::move(found);
}
void Metadata::loadMessages(const Bytes& languagePack) {
    auto pack=unpackSarc(languagePack);
    for(auto& file:pack) {
        if(file.first.find("Message/")!=0)continue;
        auto archive=unpackSarc(file.second);
        for(auto& table:archive) {
            if(table.first=="StaticMsg/LocationMarker.msbt"){locations=readMsbt(table.second);continue;}
            if(table.first.find("ActorType/")!=0 || table.first.find(".msbt")==std::string::npos)continue;
            auto labels=readMsbt(table.second);
            for(auto& label:labels)if(label.first.size()>5 && label.first.substr(label.first.size()-5)=="_Name")names.emplace(label.first.substr(0,label.first.size()-5),label.second);
        }
    }
}
std::string Metadata::itemName(const std::string& id) const { auto it=names.find(id);return it==names.end()?id:it->second; }
std::vector<std::string> Metadata::itemIds() const {
    std::vector<std::string> ids;
    for(auto& name:names)if(name.first.find("Weapon_")==0||name.first.find("Armor_")==0||name.first.find("Item_")==0||name.first.find("Obj_")==0||name.first.find("Animal_Insect_")==0)ids.push_back(name.first);
    return ids;
}
}
