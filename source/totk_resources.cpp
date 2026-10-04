// Copyright (c) 2026 StockiestThree9. MIT.
#include "totk.hpp"
#include "platform.hpp"
#include <filesystem>
#include <ctime>
#include <cstring>
#include <zstd.h>
#ifdef __SWITCH__
#include <switch.h>
#endif
namespace zse {
namespace {
[[maybe_unused]] Bytes unpackZstd(const Bytes& input,const std::map<std::string,Bytes>& dictionaries={}){
 auto size=ZSTD_getFrameContentSize(input.data(),input.size());
 if(size==ZSTD_CONTENTSIZE_ERROR||size==ZSTD_CONTENTSIZE_UNKNOWN||size>64*1024*1024)throw Error("Invalid or oversized TOTK resource.");
 Bytes output(size);unsigned id=ZSTD_getDictID_fromFrame(input.data(),input.size());const Bytes* dictionary=nullptr;
 if(id){for(auto& pair:dictionaries)if(ZSTD_getDictID_fromDict(pair.second.data(),pair.second.size())==id){dictionary=&pair.second;break;}if(!dictionary)throw Error("Missing TOTK resource dictionary.");}
 auto context=std::unique_ptr<ZSTD_DCtx,decltype(&ZSTD_freeDCtx)>(ZSTD_createDCtx(),ZSTD_freeDCtx);if(!context)throw Error("Could not allocate resource decoder.");
 auto result=ZSTD_decompress_usingDict(context.get(),output.data(),output.size(),input.data(),input.size(),dictionary?dictionary->data():nullptr,dictionary?dictionary->size():0);
 if(ZSTD_isError(result)||result!=size)throw Error("Could not decode TOTK game resource.");return output;
}
void namesFromArchive(Metadata& data,const Bytes& bytes){
 for(auto& table:unpackSarc(bytes)){
  if(table.first.size()<5||table.first.substr(table.first.size()-5)!=".msbt")continue;
  if(table.first.find("ActorType/")==0){for(auto& pair:readMsbt(table.second))if(pair.first.size()>5&&pair.first.substr(pair.first.size()-5)=="_Name")data.names.emplace(pair.first.substr(0,pair.first.size()-5),pair.second);}
  if(table.first=="LocationMsg/Location.msbt"||table.first=="StaticMsg/LocationMarker.msbt")data.locations=readMsbt(table.second);
 }
}
}
void readTotkCaption(const Bytes& bytes,Slot& slot){
 totk::Save caption(bytes);
 auto number=[&](uint32_t hash){auto& f=caption.field(hash);if(f.count!=1||(f.type!=totk::Type::Int&&f.type!=totk::Type::UInt))throw Error("Unexpected TOTK caption date field.");return caption.word(hash);};
 try{
  unsigned year=number(0x9811a3f7),month=number(0xdfd840d3),day=number(0xbd46f485),hour=number(0x23f3d75e),minute=number(0x27853bf7);
  if(year<2000||year>2100||month<1||month>12||day<1||day>31||hour>23||minute>59)throw Error("Invalid TOTK caption date.");
  std::tm date{};date.tm_year=int(year)-1900;date.tm_mon=int(month)-1;date.tm_mday=int(day);date.tm_hour=int(hour);date.tm_min=int(minute);date.tm_isdst=-1;
  auto stamp=std::mktime(&date);if(stamp>0){slot.timestamp=uint64_t(stamp);char shown[80];if(std::strftime(shown,sizeof(shown),"%b %d, %Y · %I:%M %p",&date))slot.date=shown;}
 }catch(const Error&){}
 auto automatic=caption.fields().find(0x25f03caa);
 if(automatic!=caption.fields().end()&&automatic->second.type==totk::Type::Bool){slot.automatic=bytes[automatic->second.offset]!=0;slot.label=slot.automatic?"Autosave":"Manual save";}
 auto image=caption.fields().find(0x63696a32);
 if(image!=caption.fields().end()&&image->second.type==totk::Type::Binary){auto& f=image->second;if(f.extent>=8&&f.extent<=4*1024*1024&&bytes[f.offset+4]==0xff&&bytes[f.offset+5]==0xd8)slot.image=Bytes(bytes.begin()+f.offset+4,bytes.begin()+f.offset+f.extent);}
}
void loadTotkNames(Metadata& data,const std::string& path,const std::function<void(const std::string&)>& log){
 const auto cache=path+"/cache/totk";std::filesystem::create_directories(cache);
#ifdef __SWITCH__
 bool mounted=false,raw=false;FsFileSystem fs{};Result rc=fsOpenFileSystemWithPatch(&fs,TotkId,FsFileSystemType_ContentData);
 if(R_SUCCEEDED(rc)){mounted=fsdevMountDevice("ztotk",fs)>=0;if(!mounted)fsFsClose(&fs);}
 if(!mounted){rc=romfsMountDataStorageFromProgram(TotkId,"ztotk");raw=mounted=R_SUCCEEDED(rc);}
 if(!mounted){rc=romfsMountFromCurrentProcess("ztotk");raw=mounted=R_SUCCEEDED(rc);}
 if(mounted){
  try{
   auto dictionaries=unpackSarc(unpackZstd(readFile("ztotk:/Pack/ZsDic.pack.zs")));
   bool found=false;
   for(auto& file:std::filesystem::directory_iterator("ztotk:/Mals")){
    auto name=file.path().filename().string();if(name.rfind("USen.Product.",0)!=0||file.path().extension()!=".zs")continue;
    auto archive=unpackZstd(readFile(file.path().string()),dictionaries);namesFromArchive(data,archive);
    if(data.names.empty())throw Error("TOTK item name table was empty.");writeFile(cache+"/messages.sarc",archive);found=true;break;
   }
   if(!found)throw Error("TOTK English item names are unavailable.");
  }catch(const std::exception& e){log(e.what());}
  if(raw)romfsUnmount("ztotk");else fsdevUnmountDevice("ztotk");
 }
#endif
 if(data.names.empty())try{namesFromArchive(data,readFile(cache+"/messages.sarc"));}catch(const std::exception& e){log(e.what());}
}
}
