// Copyright (c) 2026 StockiestThree9
#include "profile_swap.hpp"
#include <chrono>
#include <iostream>
using namespace zse;
namespace fs=std::filesystem;
static int checks=0;
static void require(bool value,const char* why){++checks;if(!value)throw Error(why);}
static void put(const fs::path& path,const Bytes& bytes){fs::create_directories(path.parent_path());writeFile(path.string(),bytes);}
static Bytes save(int amount){Bytes b;for(auto v:{0x4730u,0xffffffffu,1u,hashName("CurrentRupee"),uint32_t(amount),0xffffffffu})for(int i=0;i<4;++i)b.push_back(uint8_t(v>>(i*8)));return b;}
using Snapshot=std::map<std::string,Bytes>;
static Snapshot snapshot(const fs::path& root){Snapshot out;for(auto& e:fs::recursive_directory_iterator(root)){auto rel=fs::relative(e.path(),root).generic_string();if(rel=="zse-profile-swap"||rel.rfind("zse-profile-swap/",0)==0)continue;if(e.is_directory())out[rel+"/"]={};else out[rel]=readFile(e.path().string());}return out;}
static void fixture(const fs::path& root){
    put(root/"0/game_data.sav",save(10));put(root/"0/caption.jpg",{1,2,3});put(root/"0/caption.sav",save(11));put(root/"5/game_data.sav",save(15));put(root/"7/game_data.sav",save(17));
    put(root/"6/game_data.sav",save(16));put(root/"6/caption.jpg",{6,6});put(root/"7/caption.sav",{7,7});
    put(root/"album/pict_000.jpg",{1,1});put(root/"pict_book/picture.jpg",{1,2});put(root/"tracker/trackblock_hard00.sav",{1,3});put(root/"tracker/trackblock00.sav",{1,4});put(root/"tracker/other.bin",{1,5});fs::create_directories(root/"album/empty");
    put(root/"ch_01/0/game_data.sav",save(20));put(root/"ch_01/0/caption.jpg",{2,3,4});put(root/"ch_01/1/game_data.sav",save(21));
    put(root/"ch_01/album/pict_000.jpg",{2,1});put(root/"ch_01/pict_book/picture.jpg",{2,2});put(root/"ch_01/tracker/trackblock00.sav",{2,3});put(root/"ch_01/tracker/trackblock01.sav",{2,4});put(root/"ch_01/tracker/trackblock_hard00.sav",{2,5});
    // Even an unsupported Master Mode document must not participate in a normal-save swap.
    put(root/"ch_01/6/game_data.sav",{0xde,0xad,0xbe,0xef});put(root/"ch_01/6/caption.jpg",{2,6});
    put(root/"option.sav",{9,8,7});put(root/".DS_Store",{6,5});put(root/"ch_01/.DS_Store",{4,3});
}
static Snapshot protectedFiles(const fs::path& root){Snapshot out;for(const auto& name:{"6/game_data.sav","6/caption.jpg","7/game_data.sav","7/caption.sav","ch_01/6/game_data.sav","ch_01/6/caption.jpg","album/pict_000.jpg","pict_book/picture.jpg","ch_01/album/pict_000.jpg","ch_01/pict_book/picture.jpg","tracker/trackblock_hard00.sav","ch_01/tracker/trackblock_hard00.sav","tracker/other.bin","option.sav"})out[name]=readFile((root/name).string());return out;}
static void swapped(const fs::path& root){
    require(readFile((root/"0/game_data.sav").string())==save(20),"Profile 2 did not become active");
    require(readFile((root/"ch_01/0/game_data.sav").string())==save(10),"Original Profile 1 was not retained");
    require(readFile((root/"7/game_data.sav").string())==save(17),"Master Mode moved from its original location");
    require(!fs::exists(root/"5")&&!fs::exists(root/"ch_01/7")&&!fs::exists(root/"ch_01/1"),"Unrelated or absent slots were changed");
    require(readFile((root/"album/pict_000.jpg").string())==Bytes({1,1}),"Shared album was changed");
    require(readFile((root/"ch_01/pict_book/picture.jpg").string())==Bytes({2,2}),"Shared compendium was changed");
    require(readFile((root/"tracker/trackblock_hard00.sav").string())==Bytes({1,3}),"Master Mode Hero's Path moved");
    require(readFile((root/"tracker/trackblock00.sav").string())==Bytes({2,3})&&readFile((root/"ch_01/tracker/trackblock00.sav").string())==Bytes({1,4}),"Normal Hero's Path did not exchange correctly");
    require(readFile((root/"tracker/trackblock01.sav").string())==Bytes({2,4})&&!fs::exists(root/"ch_01/tracker/trackblock01.sav"),"Asymmetric normal Hero's Path was duplicated or lost");
    require(readFile((root/"option.sav").string())==Bytes({9,8,7}),"Options were changed");
    require(!profileSwapPending(root),"Completed swap still pending");
}
int main(){try{
    fs::path run="run-"+std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count());fs::create_directories(run);
    auto root=run/"normal";fixture(root);auto before=snapshot(root);auto protectedBefore=protectedFiles(root);size_t moves=0;
    auto info=inspectProfiles(root);require(info.normal[0]==2&&info.normal[1]==2,"Swap counts include non-normal saves");
    exchangeProfiles(root,run/"backup-a",[]{},[&](size_t step){moves=step+1;require(protectedFiles(root)==protectedBefore,"A protected file changed during a swap");});swapped(root);require(protectedFiles(root)==protectedBefore,"Master Mode or shared files changed");
    require(snapshot(run/"backup-a")==before,"Full backup differs from original container");
    exchangeProfiles(root,run/"backup-b",[]{});require(snapshot(root)==before,"Two swaps did not restore every original byte and directory");
    auto progress=run/"progress";fixture(progress);exchangeProfiles(progress,run/"backup-progress-a",[]{});
    put(progress/"0/game_data.sav",save(777));put(progress/"album/new-photo.jpg",{7,7,7});put(progress/"tracker/trackblock02.sav",{7,8,9});
    exchangeProfiles(progress,run/"backup-progress-b",[]{});require(readFile((progress/"ch_01/0/game_data.sav").string())==save(777),"Swapping back discarded new progress");require(readFile((progress/"album/new-photo.jpg").string())==Bytes({7,7,7}),"New shared photo was moved or lost");require(readFile((progress/"ch_01/tracker/trackblock02.sav").string())==Bytes({7,8,9}),"New normal history was lost");
    exchangeProfiles(progress,run/"backup-progress-c",[]{});require(readFile((progress/"0/game_data.sav").string())==save(777),"Returning to the second adventure lost progress");
    for(size_t stop=0;stop<moves;++stop){
        auto folder=run/("fault-"+std::to_string(stop));fixture(folder);auto original=snapshot(folder);auto protectedOriginal=protectedFiles(folder);bool stopped=false;
        try{exchangeProfiles(folder,run/("backup-fault-"+std::to_string(stop)),[]{},[&](size_t step){if(step==stop)throw Error("Simulated interruption");});}catch(const Error&){stopped=true;}
        require(stopped&&profileSwapPending(folder),"Interrupted rename not marked recoverable");
        require(protectedFiles(folder)==protectedOriginal,"Interruption changed Master Mode or shared data");
        continueProfileSwap(folder,[]{},[&](size_t){require(protectedFiles(folder)==protectedOriginal,"Recovery touched protected data");});swapped(folder);
        exchangeProfiles(folder,run/("backup-return-"+std::to_string(stop)),[]{});require(snapshot(folder)==original,"Recovery plus reverse lost data");
    }
    auto conflict=run/"conflict";fixture(conflict);try{exchangeProfiles(conflict,run/"backup-conflict",[]{},[](size_t){throw Error("Stop");});}catch(const Error&){}
    put(conflict/"0/game_data.sav",save(999));bool refused=false;try{continueProfileSwap(conflict,[]{});}catch(const Error&){refused=true;}
    require(refused&&profileSwapPending(conflict),"Recovery overwrote conflicting live data");require(readFile((conflict/"0/game_data.sav").string())==save(999),"Conflicting data was lost");
    auto unfamiliar=run/"unknown";fixture(unfamiliar);put(unfamiliar/"custom/data",{4});auto unknownBefore=snapshot(unfamiliar);refused=false;
    try{exchangeProfiles(unfamiliar,run/"backup-unknown",[]{});}catch(const Error&){refused=true;}require(refused&&snapshot(unfamiliar)==unknownBefore,"Unknown layout was partly modified");
    auto failed=run/"backup-failure";fixture(failed);auto failedBefore=snapshot(failed);put(run/"occupied",{9});refused=false;
    try{exchangeProfiles(failed,run/"occupied",[]{});}catch(const Error&){refused=true;}require(refused&&snapshot(failed)==failedBefore,"Backup failure changed live files");
    auto empty=run/"empty-first";put(empty/"ch_01/0/game_data.sav",save(30));auto emptyBefore=snapshot(empty);exchangeProfiles(empty,run/"backup-empty-a",[]{});exchangeProfiles(empty,run/"backup-empty-b",[]{});require(snapshot(empty)==emptyBefore,"Empty profile round-trip failed");
    auto legacy=run/"legacy";fixture(legacy);auto legacyBefore=snapshot(legacy);std::string v1="ZSE-PROFILE-SWAP-1\n";put(legacy/"zse-profile-swap/plan.txt",Bytes(v1.begin(),v1.end()));refused=false;try{continueProfileSwap(legacy,[]{});}catch(const Error&){refused=true;}require(refused&&snapshot(legacy)==legacyBefore,"Older whole-profile recovery touched protected data");
    std::cout<<"PASS: "<<checks<<" normal-swap checks; "<<moves<<" interruption points, Master Mode/shared-data invariance, normal-history exchange, two-way retention, backups and legacy-plan refusal\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
