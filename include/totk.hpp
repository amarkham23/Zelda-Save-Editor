// Copyright (c) 2026 StockiestThree9. MIT.
#pragma once
#include "botw.hpp"
#include "totk_format.hpp"
namespace zse {
struct Slot;
void readTotkCaption(const Bytes& bytes,Slot& slot);
void loadTotkNames(Metadata& metadata,const std::string& dataPath,const std::function<void(const std::string&)>& log);
class Totk {
public:
 explicit Totk(Metadata& metadata):metadata(metadata){}
 Metadata& metadata;
 void definitions(const Save& save);
 std::vector<EditField> stats(const Save& save) const;
 std::vector<Item> items(const Save& save) const;
 std::vector<EditField> itemFields(const Save& save,const Item& item) const;
 std::vector<EditField> matching(const Save& save,const std::function<bool(const Field&)>& predicate) const;
 std::vector<EditField> elements(const Save& save,const Field& field) const;
 std::string itemValue(const Save& save,const Item& item) const;
 std::string itemDetail(const Save& save,const Item& item) const;
private:
 EditField field(const std::string& name,const std::string& label,size_t index=0) const;
};
}
