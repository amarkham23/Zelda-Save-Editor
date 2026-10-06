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
 // Inventory operations affect the ordinary pouch only. The temporary pouch
 // occupies the second half of the game's arrays and is preserved byte-for-byte.
 size_t capacity(const Save& save,ItemGroup group) const;
 size_t availableSlots(const Save& save,ItemGroup group) const;
 std::vector<std::string> catalog(const Save& save,ItemGroup group) const;
 bool canManage(ItemGroup group) const;
 bool canManage(const Item& item) const;
 bool canDuplicate(ItemGroup group) const;
 Item addItem(Save& save,ItemGroup group,const std::string& id) const;
 void replaceItem(Save& save,const Item& item,const std::string& id) const;
 void duplicateItem(Save& save,const Item& item) const;
 void removeItem(Save& save,const Item& item) const;
 bool equipped(const Save& save,const Item& item) const;
 void equipItem(Save& save,const Item& item) const;
 void clearFusion(Save& save,const Item& item) const;
 void fillStacks(Save& save,ItemGroup group) const;
private:
 std::map<std::string,ItemGroup> originalItems;
 EditField field(const std::string& name,const std::string& label,size_t index=0) const;
};
}
