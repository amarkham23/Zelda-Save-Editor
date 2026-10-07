// Copyright (c) 2026 StockiestThree9. MIT.
#pragma once
#include "botw.hpp"
#include "totk_format.hpp"
namespace zse {
struct Slot;
void readTotkCaption(const Bytes &bytes, Slot &slot);
void loadTotkNames(Metadata &metadata, const std::string &dataPath,
                   const std::function<void(const std::string &)> &log);
void readTotkItemTable(Metadata &metadata, const Bytes &table);
void readTotkActorTable(Metadata &metadata, const Bytes &table);
void linkTotkVariants(Metadata &metadata);
void readTotkEnums(Metadata &metadata, const Bytes &dictionary);
void readTotkChallenges(Metadata &metadata, const Bytes &table);
void readTotkAttachments(Metadata &metadata, const Bytes &table);
void readTotkWorld(Metadata &metadata, const Bytes &table, bool koroks);
void readTotkPlacement(Metadata &metadata, const Bytes &table);
enum class VariantProperty { Stars, Style, Condition };
struct ItemFamily {
  std::string id, name, detail;
  std::vector<std::string> ids;
};
struct Blueprint {
  size_t slot = 0;
  int32_t index = -1;
  bool favorite = false;
  Bytes data;
  std::array<uint32_t, 6> camera{};
};
class Totk {
public:
  explicit Totk(Metadata &metadata) : metadata(metadata) {}
  Metadata &metadata;
  void definitions(const Save &save);
  std::vector<EditField> stats(const Save &save) const;
  std::vector<Item> items(const Save &save) const;
  std::vector<EditField> itemFields(const Save &save, const Item &item) const;
  std::vector<EditField> matching(const Save &save,
                                  const std::function<bool(const Field &)> &predicate) const;
  std::vector<EditField> elements(const Save &save, const Field &field) const;
  std::string itemValue(const Save &save, const Item &item) const;
  std::string itemDetail(const Save &save, const Item &item) const;
  // Inventory operations affect the ordinary pouch only. The temporary pouch
  // occupies the second half of the game's arrays and is preserved byte-for-byte.
  size_t capacity(const Save &save, ItemGroup group) const;
  size_t availableSlots(const Save &save, ItemGroup group) const;
  std::vector<std::string> catalog(const Save &save, ItemGroup group) const;
  std::vector<ItemFamily> families(const Save &save, ItemGroup group) const;
  std::string variantLabel(const std::string &id) const;
  int initialDurability(const std::string &id) const;
  std::vector<std::pair<std::string, std::string>> variantOptions(const Save &save, ItemGroup group,
                                                                  const std::string &id,
                                                                  VariantProperty property) const;
  void changeVariant(Save &save, const Item &item, const std::string &id) const;
  bool canManage(ItemGroup group) const;
  bool canManage(const Item &item) const;
  bool canDuplicate(ItemGroup group) const;
  Item addItem(Save &save, ItemGroup group, const std::string &id) const;
  void replaceItem(Save &save, const Item &item, const std::string &id) const;
  void duplicateItem(Save &save, const Item &item) const;
  void removeItem(Save &save, const Item &item) const;
  bool equipped(const Save &save, const Item &item) const;
  void equipItem(Save &save, const Item &item) const;
  void clearFusion(Save &save, const Item &item) const;
  void fillStacks(Save &save, ItemGroup group) const;
  std::vector<std::string> fusionCatalog(const Save &save) const;
  void setFusion(Save &save, const Item &item, const std::string &attachment) const;
  void repairEquipment(Save &save, ItemGroup group) const;
  void upgradeArmor(Save &save) const;
  void restorePristine(Save &save) const;
  void unlockPristine(Save &save) const;
  std::vector<EditField> horseFields(const Save &save, size_t horse) const;
  std::vector<EditField> mapFields(const Save &save, size_t stamp) const;
  void clearMapStamps(Save &save) const;
  void resetAmiibo(Save &save) const;
  std::vector<EditField> questFields(const Save &save, const std::string &id) const;
  Blueprint blueprint(const Save &save, size_t slot) const;
  void importBlueprint(Save &save, size_t slot, const Bytes &file) const;
  Bytes exportBlueprint(const Save &save, size_t slot) const;
  void favoriteBlueprint(Save &save, size_t slot) const;
  std::vector<EditField> worldFields(const Save &save, const std::string &category) const;
  void completeWorld(Save &save, const std::string &category) const;
  void unlockCompendium(Save &save) const;
  std::vector<EditField> abilities(const Save &save) const;
  void setAbility(Save &save, const std::string &id, bool enabled) const;

private:
  std::map<std::string, ItemGroup> originalItems;
  EditField field(const std::string &name, const std::string &label, size_t index = 0) const;
};
} // namespace zse
