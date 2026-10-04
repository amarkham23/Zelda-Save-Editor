// Copyright (c) 2026 StockiestThree9. MIT.
// Original reader implementing the public TOTK SAV format; no editor source is vendored.
#pragma once
#include "save.hpp"
#include <array>
#include <set>

namespace zse::totk {
uint32_t hash(const std::string& text);
enum class Type : uint32_t {
 Bool,BoolArray,Int,IntArray,Float,FloatArray,Enum,EnumArray,
 Vector2,Vector2Array,Vector3,Vector3Array,String16,String16Array,
 String32,String32Array,String64,String64Array,Binary,BinaryArray,
 UInt,UIntArray,Int64,Int64Array,UInt64,UInt64Array,
 WString16,WString16Array,WString32,WString32Array,WString64,WString64Array,Bool64Key
};
struct Field {Type type;size_t entry=0,offset=0,count=1,width=0,extent=0;};
class Save {
public:
 explicit Save(zse::Bytes data);
 uint32_t version() const{return revision_;}
 const zse::Bytes& bytes() const{return data_;}
 const std::map<uint32_t,Field>& fields() const{return fields_;}
 const Field* find(const std::string& name) const;
 const Field& field(uint32_t hash) const;
 uint32_t word(uint32_t hash,size_t index=0) const;
 int32_t integer(const std::string& name,size_t index=0) const;
 float real(const std::string& name,size_t index=0) const;
 bool boolean(const std::string& name,size_t index=0) const;
 std::array<float,3> vector3(const std::string& name,size_t index=0) const;
 std::string string(const std::string& name,size_t index=0) const;
 bool hasObjectFlag(uint64_t value) const{return objectFlags_.count(value)!=0;}
 size_t objectFlagCount() const{return objectFlags_.size();}
private:
 zse::Bytes data_;uint32_t revision_=0;size_t dataStart_=0;
 std::map<uint32_t,Field> fields_;std::set<uint64_t> objectFlags_;
 void bounds(size_t offset,size_t size) const;
 uint32_t u32(size_t offset) const;
 uint64_t u64(size_t offset) const;
 float f32(size_t offset) const;
 size_t at(const Field& f,size_t index) const;
 void validate(Field& f,size_t limit);
};
struct Summary {
 int32_t rupees=0;float hearts=0,staminaWheels=0;
 std::array<float,3> position{};std::string map;
 unsigned shrinesPresent=0,shrinesCompleted=0;
 unsigned lightrootsPresent=0,lightrootsActivated=0;
 unsigned towersPresent=0,towersActivated=0;
};
Summary summarize(const Save& save);
}
