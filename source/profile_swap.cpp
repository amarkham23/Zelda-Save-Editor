// Copyright (c) 2026 StockiestThree9
#include "profile_swap.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>

namespace zse {
namespace fs=std::filesystem;
namespace {
constexpr const char* Tx="zse-profile-swap";
struct Entry {std::string path;bool directory;uint64_t size;uint32_t crc;};
using Tree=std::vector<Entry>;
const std::vector<std::string> NormalSlots={"0","1","2","3","4","5"};
bool normalSlot(const std::string& name){return std::find(NormalSlots.begin(),NormalSlots.end(),name)!=NormalSlots.end();}
bool normalTrack(const std::string& name){
    const std::string prefix="tracker/trackblock",suffix=".sav";
    if(name.rfind(prefix,0)!=0||name.size()<prefix.size()+1+suffix.size()||name.size()>prefix.size()+4+suffix.size()||name.compare(name.size()-suffix.size(),suffix.size(),suffix)!=0)return false;
    auto number=name.substr(prefix.size(),name.size()-prefix.size()-suffix.size());return number.find_first_not_of("0123456789")==std::string::npos;
}
bool normalPath(const std::string& name){return normalSlot(name)||normalTrack(name);}
std::string token(std::string path){std::replace(path.begin(),path.end(),'/','_');return path;}
bool knownRoot(const std::string& name){return normalSlot(name)||name=="6"||name=="7"||name=="album"||name=="pict_book"||name=="tracker";}
Tree tree(const fs::path& root){
    Tree result;uint64_t total=0;
    std::function<void(const fs::path&,const std::string&,int)> visit=[&](const fs::path& dir,const std::string& prefix,int depth){
        if(depth>24)throw Error("A save folder is nested too deeply.");
        for(auto& e:fs::directory_iterator(dir)){
            if(e.is_symlink())throw Error("Links are not supported inside save data.");
            auto rel=prefix+e.path().filename().generic_string();
            if(e.is_directory()){result.push_back({rel,true,0,0});visit(e.path(),rel+"/",depth+1);}
            else if(e.is_regular_file()){auto b=readFile(e.path().string());total+=b.size();if(total>1024ULL*1024*1024)throw Error("This save is too large for a profile swap.");result.push_back({rel,false,b.size(),hashName(std::string(b.begin(),b.end()))});}
            else throw Error("Unsupported file in save data.");
            if(result.size()>20000)throw Error("This save contains too many files.");
        }
    };
    if(!fs::is_directory(root))throw Error("A profile folder is missing.");
    visit(root,"",0);std::sort(result.begin(),result.end(),[](auto& a,auto& b){return a.path<b.path;});return result;
}
std::string describe(const Tree& entries){std::ostringstream s;for(auto& e:entries)s<<(e.directory?'D':'F')<<' '<<std::quoted(e.path)<<' '<<e.size<<' '<<e.crc<<'\n';return s.str();}
std::string signature(const fs::path& root){if(fs::is_regular_file(root)){auto b=readFile(root.string());return "F "+std::to_string(b.size())+" "+std::to_string(hashName(std::string(b.begin(),b.end())));}return "D\n"+describe(tree(root));}
std::string protectedData(const fs::path& root){
    auto entries=tree(root);Tree keep;
    for(auto& e:entries){auto path=e.path;if(path==Tx||path.rfind(std::string(Tx)+"/",0)==0)continue;
        if(path.rfind("ch_01/",0)==0)path=path.substr(6);
        auto slash=path.find('/');auto first=path.substr(0,slash);
        if(normalSlot(first)||normalTrack(path)||path=="tracker")continue;
        keep.push_back(e);
    }
    return describe(keep);
}
void textFile(const fs::path& path,const std::string& text){writeFile(path.string(),Bytes(text.begin(),text.end()));}
std::string textFile(const fs::path& path){auto b=readFile(path.string(),8*1024*1024);return std::string(b.begin(),b.end());}
void verifiedBackup(const fs::path& root,const fs::path& backup){
    if(fs::exists(backup))throw Error("The profile backup folder already exists. Please try again.");
    auto entries=tree(root);fs::create_directories(backup);
    for(auto& e:entries){auto dst=backup/e.path;if(e.directory)fs::create_directories(dst);else{fs::create_directories(dst.parent_path());auto b=readFile((root/e.path).string());writeFile(dst.string(),b);if(readFile(dst.string())!=b)throw Error("Backup verification failed. No profiles were moved.");}}
    if(describe(tree(root))!=describe(entries)||describe(tree(backup))!=describe(entries))throw Error("Save data changed while backing up. No profiles were moved.");
}
struct Move {std::string from,to,expected;};
struct Plan {std::string backup,preserved;std::vector<std::string> scope;std::vector<Move> moves;std::map<std::string,std::string> final;};
std::string serialized(const Plan& plan){std::ostringstream out;out<<"ZSE-NORMAL-SWAP-2\n"<<std::quoted(plan.backup)<<'\n'<<std::quoted(plan.preserved)<<'\n'<<plan.scope.size()<<'\n';for(auto& n:plan.scope)out<<std::quoted(n)<<'\n';out<<plan.moves.size()<<'\n';for(auto& m:plan.moves)out<<std::quoted(m.from)<<' '<<std::quoted(m.to)<<' '<<std::quoted(m.expected)<<'\n';out<<plan.final.size()<<'\n';for(auto& f:plan.final)out<<std::quoted(f.first)<<' '<<std::quoted(f.second)<<'\n';return out.str();}
bool movePath(const std::string& path){
    if(normalPath(path))return true;
    if(path.rfind("ch_01/",0)==0)return normalPath(path.substr(6));
    const std::string hold=std::string(Tx)+"/hold/";
    if(path.rfind(hold,0)==0){auto n=path.substr(hold.size());if(normalSlot(n))return true;if(n.rfind("tracker_",0)==0)return normalTrack("tracker/"+n.substr(8));}
    return false;
}
Plan readPlan(const fs::path& root){
    auto input=textFile(root/Tx/"plan.txt");std::istringstream in(input);std::string magic;std::getline(in,magic);
    if(magic=="ZSE-PROFILE-SWAP-1")throw Error("This unfinished operation was created by version 0.1.3. Recover it with that version before using normal-save swaps. Its data and full backup are retained.");
    if(magic!="ZSE-NORMAL-SWAP-2")throw Error("The profile swap record is damaged. Your full SD backup is retained.");
    Plan p;size_t count=0;in>>std::quoted(p.backup)>>std::quoted(p.preserved)>>count;if(!in||count<6||count>1024)throw Error("Invalid normal-save swap record.");
    std::set<std::string> scope;
    for(size_t i=0;i<count;++i){std::string n;in>>std::quoted(n);if(!in||!normalPath(n)||!scope.insert(n).second)throw Error("Invalid normal-save location.");p.scope.push_back(n);}
    for(auto& n:NormalSlots)if(!scope.count(n))throw Error("Incomplete normal-save swap record.");
    in>>count;if(!in||count>p.scope.size()*3)throw Error("Invalid profile swap record.");
    for(size_t i=0;i<count;++i){Move m;in>>std::quoted(m.from)>>std::quoted(m.to)>>std::quoted(m.expected);if(!in||!movePath(m.from)||!movePath(m.to)||m.from==m.to)throw Error("Invalid profile swap location.");p.moves.push_back(std::move(m));}
    in>>count;if(!in||count>p.scope.size()*2)throw Error("Invalid profile swap result record.");
    for(size_t i=0;i<count;++i){std::string name,value;in>>std::quoted(name)>>std::quoted(value);if(!in||!movePath(name)||name.rfind(Tx,0)==0)throw Error("Invalid profile swap result location.");p.final.emplace(name,value);}
    in>>std::ws;if(!in.eof())throw Error("Unexpected data in profile swap record.");return p;
}
void cleanFinished(const fs::path& root){
    auto tx=root/Tx;if(!fs::exists(tx))return;
    if(!fs::is_regular_file(tx/"complete"))throw Error("Resume the interrupted profile swap first.");
    // Remove only our completed journal and empty staging directory; never recursively remove save data.
    if(fs::exists(tx/"hold")&&!fs::is_empty(tx/"hold"))throw Error("Unexpected staged save data was retained. Recovery is required.");
    for(auto& e:fs::directory_iterator(tx)){auto n=e.path().filename().string();if(n=="hold")continue;if(!e.is_regular_file()||(n!="complete"&&n!="plan.txt"&&n.rfind("step-",0)!=0))throw Error("Unexpected data in the completed swap record.");}
    std::vector<fs::path> remove;for(auto& e:fs::directory_iterator(tx))if(e.path().filename()!="complete")remove.push_back(e.path());
    for(auto& path:remove)fs::remove(path);
    fs::remove(tx/"complete");fs::remove(tx);
}
void validateLayout(const fs::path& root){
    for(auto& e:fs::directory_iterator(root)){
        auto n=e.path().filename().string();
        if(n=="ch_01"||n=="option.sav"||n==Tx||n==".DS_Store"||n.rfind("._",0)==0)continue;
        if(!knownRoot(n))throw Error("An unfamiliar file or folder prevents a normal-save swap: "+n+". No data was moved.");
    }
    if(!fs::is_directory(root/"ch_01"))throw Error("No second game profile was found.");
    for(auto& e:fs::directory_iterator(root/"ch_01")){auto n=e.path().filename().string();if(!knownRoot(n)&&n!=".DS_Store"&&n.rfind("._",0)!=0)throw Error("An unfamiliar Profile 2 file prevents a normal-save swap: "+n);}
    for(const auto& prefix:{std::string(),std::string("ch_01/")})for(int i=0;i<6;++i){auto dir=root/(prefix+std::to_string(i));if(fs::exists(dir)&&!fs::is_directory(dir))throw Error("A normal-save folder is invalid.");auto file=dir/"game_data.sav";if(fs::exists(file)){Save save(readFile(file.string()));if(save.bigEndian())throw Error("A profile contains a save for another platform.");}}
}
}
bool profileSwapPending(const fs::path& root){return fs::exists(root/Tx)&&!fs::is_regular_file(root/Tx/"complete");}
SwapInfo inspectProfiles(const fs::path& root){SwapInfo result;result.pending=profileSwapPending(root);for(int p=0;p<2;++p)for(int s=0;s<6;++s)if(fs::is_regular_file(root/(p?"ch_01":"")/std::to_string(s)/"game_data.sav"))++result.normal[p];return result;}
std::string continueProfileSwap(const fs::path& root,const std::function<void()>& commit,const std::function<void(size_t)>& checkpoint){
    auto tx=root/Tx;auto plan=readPlan(root);
    if(fs::is_regular_file(tx/"complete"))return plan.backup;
    if(protectedData(root)!=plan.preserved)throw Error("Data outside the normal-save swap changed. Recovery stopped without overwriting it.");
    bool gap=false;
    for(size_t i=0;i<plan.moves.size();++i){
        auto marker=tx/("step-"+std::to_string(i));auto& m=plan.moves[i];
        if(fs::exists(marker)){if(gap)throw Error("The profile swap record is inconsistent. Data has been retained.");continue;}
        gap=true;auto from=root/m.from,to=root/m.to;
        if(fs::exists(from)&&!fs::exists(to)){if(signature(from)!=m.expected)throw Error("A profile changed during the swap. Data and backups have been retained.");fs::create_directories(to.parent_path());fs::rename(from,to);}
        else if(fs::exists(from)||!fs::exists(to)||signature(to)!=m.expected)throw Error("Could not safely resume the profile swap. Data and backups have been retained.");
        // Fault injection exercises recovery after a rename but before its completion marker.
        if(checkpoint)checkpoint(i);
        textFile(marker,"done\n");commit();
    }
    for(const auto& name:plan.scope)for(const auto& prefix:{std::string(),std::string("ch_01/")}){
        auto location=prefix+name;auto expected=plan.final.find(location);auto path=root/location;
        if(expected==plan.final.end()){if(fs::exists(path))throw Error("Unexpected data after the profile swap. Recovery record retained.");}
        else if(!fs::exists(path)||signature(path)!=expected->second)throw Error("Profile swap verification failed. Data and backups have been retained.");
    }
    if(!fs::is_empty(tx/"hold"))throw Error("Staged data remains. Recovery record retained.");
    if(protectedData(root)!=plan.preserved)throw Error("Data outside the normal-save swap changed. Recovery record retained.");
    textFile(tx/"complete","verified\n");commit();return plan.backup;
}
std::string exchangeProfiles(const fs::path& root,const fs::path& backup,const std::function<void()>& commit,const std::function<void(size_t)>& checkpoint){
    if(profileSwapPending(root))throw Error("Resume the interrupted profile swap first.");
    validateLayout(root);auto info=inspectProfiles(root);if(!info.normal[0]&&!info.normal[1])throw Error("No normal-mode saves were found to swap.");
    cleanFinished(root);commit();
    verifiedBackup(root,backup);
    Plan plan;plan.backup=backup.generic_string();plan.preserved=protectedData(root);plan.scope=NormalSlots;
    std::set<std::string> tracks;
    for(const auto& prefix:{std::string(),std::string("ch_01/")}){auto path=root/(prefix+"tracker");if(fs::is_directory(path))for(auto& e:fs::directory_iterator(path)){auto name="tracker/"+e.path().filename().generic_string();if(e.is_regular_file()&&normalTrack(name))tracks.insert(name);}}
    plan.scope.insert(plan.scope.end(),tracks.begin(),tracks.end());
    for(const auto& n:plan.scope){
        auto a=root/n,b=root/"ch_01"/n;std::string hold=std::string(Tx)+"/hold/"+token(n);
        if(fs::exists(a)){auto sig=signature(a);plan.moves.push_back({n,hold,sig});plan.final["ch_01/"+n]=sig;}
        if(fs::exists(b)){auto sig=signature(b);plan.moves.push_back({"ch_01/"+n,n,sig});plan.final[n]=sig;}
        if(fs::exists(a))plan.moves.push_back({hold,"ch_01/"+n,plan.final["ch_01/"+n]});
    }
    fs::create_directories(root/Tx/"hold");textFile(root/Tx/"plan.txt",serialized(plan));commit();
    return continueProfileSwap(root,commit,checkpoint);
}
}
