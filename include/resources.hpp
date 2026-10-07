// Copyright (c) 2026 StockiestThree9
#pragma once
#include "save.hpp"
#include <array>
#include <memory>
#include <unordered_map>

namespace zse {
Bytes decompress(const Bytes& data);
std::map<std::string, Bytes> unpackSarc(const Bytes& data);
enum class Kind { Unknown, Bool, Int, UInt, Float, String, Vec2, Vec3, Vec4, WideString };
struct Field {
    std::string name;
    uint32_t hash=0;
    Kind kind=Kind::Unknown;
    size_t width=1;
    bool array=false;
};
// Facts read from the installed game's item tables, never a bundled item DB.
struct ItemVariant {
    std::string family, nextArmor, style, condition;
    std::string category, conversion, alias, model, modelProject;
    int stars=-1;
    int durability=0,performance=-1;
    bool armor=false;
};
struct WorldEntry { std::string category,id,field; std::array<float,3> position{}; };
struct Metadata {
    std::map<uint32_t,Field> fields;
    std::map<std::string,std::string> names;
    std::map<std::string,std::string> locations;
    std::map<std::string,ItemVariant> variants;
    std::map<uint32_t,std::vector<std::pair<uint32_t,std::string>>> enums;
    std::map<std::string,std::vector<std::string>> quests;
    std::map<std::string,std::string> questNames;
    std::map<std::string,int> attachments;
    std::vector<WorldEntry> world;
    void loadGameData(const Bytes& bootup);
    void loadMessages(const Bytes& languagePack);
    std::string itemName(const std::string& id) const;
    std::vector<std::string> itemIds() const;
};
// Archive readers consume data from the user's installed game at runtime.
struct ByNode {
    uint8_t type=0xff;
    uint32_t number=0;
    uint64_t wide=0;
    std::string text;
    std::vector<ByNode> array;
    std::map<std::string,ByNode> dict;
    const ByNode* get(const std::string& key) const;
};
ByNode readByml(const Bytes& bytes);
std::map<std::string,std::string> readMsbt(const Bytes& bytes);
}
