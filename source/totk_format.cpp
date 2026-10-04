// Copyright (c) 2026 StockiestThree9. MIT.
#include "totk_format.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <limits>

namespace zse::totk {
namespace {
uint32_t rol(uint32_t v,unsigned n){return (v<<n)|(v>>(32-n));}
bool inlineType(Type t){return t==Type::Bool||t==Type::Int||t==Type::Float||t==Type::Enum||t==Type::UInt;}
size_t width(Type t){
 switch(Type(uint32_t(t)&~1U)){
  case Type::Bool:return 0;
  case Type::Int:case Type::Float:case Type::Enum:case Type::UInt:return 4;
  case Type::Vector2:case Type::Int64:case Type::UInt64:return 8;
  case Type::Vector3:return 12;
  case Type::String16:return 16;case Type::String32:return 32;case Type::String64:return 64;
  case Type::WString16:return 32;case Type::WString32:return 64;case Type::WString64:return 128;
  default:return 0;
 }
}
bool isArray(Type t){return uint32_t(t)<32&&(uint32_t(t)&1);}
void utf8(std::string& out,uint32_t cp){
 if(cp<0x80)out+=char(cp);
 else if(cp<0x800){out+=char(0xc0|(cp>>6));out+=char(0x80|(cp&63));}
 else if(cp<0x10000){out+=char(0xe0|(cp>>12));out+=char(0x80|((cp>>6)&63));out+=char(0x80|(cp&63));}
 else {out+=char(0xf0|(cp>>18));out+=char(0x80|((cp>>12)&63));out+=char(0x80|((cp>>6)&63));out+=char(0x80|(cp&63));}
}
}
// MurmurHash3 x86_32, seed zero. TOTK field identifiers use this rather than CRC32.
uint32_t hash(const std::string& text){
 uint32_t h=0,k=0;size_t p=0;
 auto mix=[](uint32_t v){return rol(v*0xcc9e2d51U,15)*0x1b873593U;};
 for(;p+4<=text.size();p+=4){k=0;for(unsigned j=0;j<4;++j)k|=uint32_t(uint8_t(text[p+j]))<<(j*8);h^=mix(k);h=rol(h,13)*5+0xe6546b64U;}
 k=0;for(size_t j=0;p+j<text.size();++j)k|=uint32_t(uint8_t(text[p+j]))<<(j*8);
 if(p<text.size())h^=mix(k);h^=uint32_t(text.size());h^=h>>16;h*=0x85ebca6bU;h^=h>>13;h*=0xc2b2ae35U;return h^(h>>16);
}
void Save::bounds(size_t p,size_t n) const{if(p>data_.size()||n>data_.size()-p)throw zse::Error("TOTK save contains an out-of-range value.");}
uint32_t Save::u32(size_t p) const{bounds(p,4);uint32_t n=0;for(unsigned i=0;i<4;++i)n|=uint32_t(data_[p+i])<<(8*i);return n;}
uint64_t Save::u64(size_t p) const{return uint64_t(u32(p))|(uint64_t(u32(p+4))<<32);}
float Save::f32(size_t p) const{uint32_t bits=u32(p);float v;std::memcpy(&v,&bits,4);if(!std::isfinite(v))throw zse::Error("TOTK value is not a finite number.");return v;}
Save::Save(zse::Bytes bytes):data_(std::move(bytes)){
 if(data_.size()<40||data_.size()>32*1024*1024||u32(0)!=0x01020304)throw zse::Error("This is not a TOTK SAV file.");
 revision_=u32(4);dataStart_=u32(8);
 if(dataStart_<40||dataStart_>data_.size()||(dataStart_-32)%8)throw zse::Error("Invalid TOTK entry-table boundary.");
 int section=-1;std::vector<size_t> offsets{data_.size()};
 for(size_t p=32;p<dataStart_;p+=8){
  uint32_t h=u32(p),v=u32(p+4);
  if(!h){if(v>32||int(v)<=section)throw zse::Error("Unknown or unordered TOTK field section.");section=int(v);continue;}
  if(section<0)throw zse::Error("Missing TOTK field section.");
  Field f{Type(section),p,p+4,1,4,4};
  if(!inlineType(f.type)){
   f.offset=v;
   if(f.type==Type::Bool64Key&&v==0){f.count=f.extent=0;}
   else {if(v<dataStart_||v%4||v>=data_.size())throw zse::Error("Invalid TOTK indirect-data offset.");offsets.push_back(v);}
  }
  if(!fields_.emplace(h,f).second)throw zse::Error("Duplicate TOTK field identifier.");
 }
 if(fields_.empty())throw zse::Error("TOTK save has no fields.");
 std::sort(offsets.begin(),offsets.end());offsets.erase(std::unique(offsets.begin(),offsets.end()),offsets.end());
 for(auto& entry:fields_){auto& f=entry.second;if(inlineType(f.type))continue;if(f.type==Type::Bool64Key&&!f.offset)continue;
  auto end=std::upper_bound(offsets.begin(),offsets.end(),f.offset);validate(f,*end);
 }
}
void Save::validate(Field& f,size_t limit){
 const size_t start=f.offset;
 auto fits=[&](size_t p,size_t n){if(p>limit||n>limit-p)throw zse::Error("TOTK field exceeds its data allocation.");bounds(p,n);};
 if(f.type==Type::Bool64Key){
  f.count=0;size_t p=start;bool terminated=false;
  while(p<=limit&&limit-p>=8){auto key=u64(p);p+=8;if(!key){terminated=true;break;}objectFlags_.insert(key);++f.count;}
  if(!terminated)throw zse::Error("TOTK object-flag list is missing its terminator.");f.width=8;f.extent=p-start;return;
 }
 f.width=width(f.type);
 if(isArray(f.type)){fits(start,4);f.count=u32(start);f.offset+=4;}
 if(f.type==Type::BoolArray){f.width=0;size_t bytes=(uint64_t(f.count)+31)/32*4;fits(f.offset,bytes);f.extent=4+bytes;return;}
 if(f.type==Type::Binary||f.type==Type::BinaryArray){
  size_t p=f.offset;
  if(f.count>(limit-p)/4)throw zse::Error("TOTK binary-array count is out of range.");
  for(size_t i=0;i<f.count;++i){fits(p,4);size_t n=u32(p);p+=4;fits(p,n);p+=n;}
  f.extent=p-start;return;
 }
 if(!f.width||f.count>(limit-f.offset)/f.width)throw zse::Error("TOTK array count is out of range.");
 fits(f.offset,f.count*f.width);f.extent=f.offset-start+f.count*f.width;
}
const Field* Save::find(const std::string& name) const{auto i=fields_.find(hash(name));return i==fields_.end()?nullptr:&i->second;}
const Field& Save::field(uint32_t h) const{auto i=fields_.find(h);if(i==fields_.end())throw zse::Error("That TOTK field is absent from this save.");return i->second;}
size_t Save::at(const Field& f,size_t i) const{if(i>=f.count)throw zse::Error("TOTK array index is out of range.");return f.offset+i*f.width;}
uint32_t Save::word(uint32_t h,size_t i) const{
 auto& f=field(h);if(f.type!=Type::Int&&f.type!=Type::IntArray&&f.type!=Type::UInt&&f.type!=Type::UIntArray&&f.type!=Type::Enum&&f.type!=Type::EnumArray)throw zse::Error("TOTK field is not an integer or enum.");return u32(at(f,i));
}
int32_t Save::integer(const std::string& name,size_t i) const{auto& f=field(hash(name));if(f.type!=Type::Int&&f.type!=Type::IntArray)throw zse::Error("TOTK field is not a signed integer.");auto bits=u32(at(f,i));int32_t v;std::memcpy(&v,&bits,4);return v;}
float Save::real(const std::string& name,size_t i) const{auto& f=field(hash(name));if(f.type!=Type::Float&&f.type!=Type::FloatArray)throw zse::Error("TOTK field is not a float.");return f32(at(f,i));}
bool Save::boolean(const std::string& name,size_t i) const{
 auto& f=field(hash(name));if(i>=f.count)throw zse::Error("TOTK bool index is out of range.");
 if(f.type==Type::Bool)return u32(f.offset)!=0;
 if(f.type==Type::BoolArray)return (u32(f.offset+(i/32)*4)>>(i%32))&1;
 throw zse::Error("TOTK field is not a boolean.");
}
std::array<float,3> Save::vector3(const std::string& name,size_t i) const{auto& f=field(hash(name));if(f.type!=Type::Vector3&&f.type!=Type::Vector3Array)throw zse::Error("TOTK field is not a position vector.");size_t p=at(f,i);return {f32(p),f32(p+4),f32(p+8)};}
std::string Save::string(const std::string& name,size_t i) const{
 auto& f=field(hash(name));auto t=Type(uint32_t(f.type)&~1U);
 if(t!=Type::String16&&t!=Type::String32&&t!=Type::String64&&t!=Type::WString16&&t!=Type::WString32&&t!=Type::WString64)throw zse::Error("TOTK field is not a string.");
 size_t start=at(f,i);std::string out;bool wide=uint32_t(t)>=26;
 for(size_t p=0;p<f.width;p+=wide?2:1){
  uint32_t ch=data_[start+p];if(wide)ch|=uint32_t(data_[start+p+1])<<8;if(!ch)return out;
  if(!wide){out+=char(ch);continue;}
  if(ch>=0xd800&&ch<=0xdbff){if(p+4>f.width)throw zse::Error("Incomplete TOTK UTF-16 string.");uint32_t lo=data_[start+p+2]|uint32_t(data_[start+p+3])<<8;if(lo<0xdc00||lo>0xdfff)throw zse::Error("Invalid TOTK UTF-16 string.");ch=0x10000+((ch-0xd800)<<10)+(lo-0xdc00);p+=2;}
  else if(ch>=0xdc00&&ch<=0xdfff)throw zse::Error("Invalid TOTK UTF-16 string.");utf8(out,ch);
 }
 return out;
}
Summary summarize(const Save& save){
 Summary s;s.rupees=save.integer("PlayerStatus.CurrentRupee");s.hearts=save.integer("PlayerStatus.MaxLife")/4.f;s.staminaWheels=save.real("PlayerStatus.MaxStamina")/1000.f;
 s.position=save.vector3("PlayerStatus.SavePos");s.map=save.string("Sequence_CurrentBanc");
 for(unsigned i=0;i<152;++i){char name[64];snprintf(name,sizeof(name),"DungeonState.Dungeon%03u",i);if(save.find(name)){++s.shrinesPresent;s.shrinesCompleted+=save.word(hash(name))==hash("Clear");}}
 for(unsigned i=0;i<152;++i){char name[64];snprintf(name,sizeof(name),"ArrivalPointState.CheckPoint%03u",i);if(save.find(name)){++s.lightrootsPresent;s.lightrootsActivated+=save.word(hash(name))==hash("Open");}}
 for(unsigned i=1;i<=15;++i){char name[64];snprintf(name,sizeof(name),"IsActivateCannon.Tower%02u",i);if(save.find(name)){++s.towersPresent;s.towersActivated+=save.boolean(name);}}
 return s;
}
}
