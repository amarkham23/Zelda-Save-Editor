// Copyright (c) 2026 StockiestThree9. MIT.
#include "platform.hpp"
#include "totk.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <zstd.h>
#ifdef __SWITCH__
#include <switch.h>
#endif
namespace zse {
namespace {
[[maybe_unused]] Bytes unpackZstd(const Bytes &input,
                                  const std::map<std::string, Bytes> &dictionaries = {}) {
  auto size = ZSTD_getFrameContentSize(input.data(), input.size());
  if (size == ZSTD_CONTENTSIZE_ERROR || size == ZSTD_CONTENTSIZE_UNKNOWN || size > 64 * 1024 * 1024)
    throw Error("Invalid or oversized TOTK resource.");
  Bytes output(size);
  unsigned id = ZSTD_getDictID_fromFrame(input.data(), input.size());
  const Bytes *dictionary = nullptr;
  if (id) {
    for (auto &pair : dictionaries)
      if (ZSTD_getDictID_fromDict(pair.second.data(), pair.second.size()) == id) {
        dictionary = &pair.second;
        break;
      }
    if (!dictionary)
      throw Error("Missing TOTK resource dictionary.");
  }
  auto context =
      std::unique_ptr<ZSTD_DCtx, decltype(&ZSTD_freeDCtx)>(ZSTD_createDCtx(), ZSTD_freeDCtx);
  if (!context)
    throw Error("Could not allocate resource decoder.");
  auto result = ZSTD_decompress_usingDict(context.get(), output.data(), output.size(), input.data(),
                                          input.size(), dictionary ? dictionary->data() : nullptr,
                                          dictionary ? dictionary->size() : 0);
  if (ZSTD_isError(result) || result != size)
    throw Error("Could not decode TOTK game resource.");
  return output;
}
void namesFromArchive(Metadata &data, const Bytes &bytes) {
  for (auto &table : unpackSarc(bytes)) {
    if (table.first.size() < 5 || table.first.substr(table.first.size() - 5) != ".msbt")
      continue;
    if (table.first.find("ActorMsg/") == 0 || table.first.find("ActorType/") == 0) {
      for (auto &pair : readMsbt(table.second))
        if (pair.first.size() > 5 && pair.first.substr(pair.first.size() - 5) == "_Name")
          data.names.emplace(pair.first.substr(0, pair.first.size() - 5), pair.second);
    }
    if (table.first == "LocationMsg/Location.msbt" ||
        table.first == "StaticMsg/LocationMarker.msbt")
      data.locations = readMsbt(table.second);
    if (table.first.rfind("ChallengeMsg/", 0) == 0) {
      auto labels = readMsbt(table.second);
      auto found = labels.find("Name");
      if (found != labels.end())
        data.questNames[std::filesystem::path(table.first).stem().string()] = found->second;
    }
  }
}
void readBubbuls(Metadata &data, const std::string &path) {
  std::ifstream file(path);
  std::string header;
  if (!std::getline(file, header) || header != "ZSEB1")
    throw Error("Bubbulfrog cache is unavailable.");
  std::vector<WorldEntry> next;
  std::set<std::string> ids;
  std::string line;
  while (std::getline(file, line)) {
    if (line.size() > 256 || next.size() >= 500)
      throw Error("Invalid Bubbulfrog cache.");
    std::istringstream row(line);
    WorldEntry point;
    point.category = "Bubbulfrogs";
    if (!(row >> point.id >> point.position[0] >> point.position[1] >> point.position[2]) ||
        point.id.size() > 127 ||
        point.id.find_first_not_of(
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_") !=
            std::string::npos ||
        !ids.insert(point.id).second)
      throw Error("Invalid Bubbulfrog cache record.");
    for (auto value : point.position)
      if (!std::isfinite(value) || std::abs(value) > 100000)
        throw Error("Invalid Bubbulfrog position.");
    point.field = "IsGetCaveMasterMedal." + point.id;
    next.push_back(std::move(point));
  }
  if (next.empty())
    throw Error("Empty Bubbulfrog cache.");
  data.world.insert(data.world.end(), next.begin(), next.end());
}
[[maybe_unused]] void writeBubbuls(const Metadata &data, const std::string &path) {
  std::ofstream file(path);
  if (!file)
    throw Error("Cannot cache Bubbulfrog records.");
  file << "ZSEB1\n";
  for (auto &p : data.world)
    if (p.category == "Bubbulfrogs")
      file << p.id << ' ' << p.position[0] << ' ' << p.position[1] << ' ' << p.position[2] << '\n';
  file.flush();
  if (!file)
    throw Error("Cannot flush Bubbulfrog cache.");
}
[[maybe_unused]] Bytes catalogBundle(const std::vector<Bytes> &tables) {
  Bytes bytes = {'Z', 'C', 'A', '1'};
  for (auto &table : tables) {
    uint32_t size = uint32_t(table.size());
    for (unsigned b = 0; b < 4; ++b)
      bytes.push_back(uint8_t(size >> (b * 8)));
    bytes.insert(bytes.end(), table.begin(), table.end());
  }
  return bytes;
}
void catalogFromBundle(Metadata &data, const Bytes &bytes) {
  if (bytes.size() < 4 || std::memcmp(bytes.data(), "ZCA1", 4))
    throw Error("Invalid item catalogue cache.");
  size_t at = 4;
  Metadata next;
  for (unsigned i = 0; i < 3; ++i) {
    if (at > bytes.size() || bytes.size() - at < 4)
      throw Error("Incomplete catalogue cache.");
    uint32_t size = 0;
    for (unsigned b = 0; b < 4; ++b)
      size |= uint32_t(bytes[at++]) << (b * 8);
    if (size > 32 * 1024 * 1024 || size > bytes.size() - at)
      throw Error("Invalid catalogue table size.");
    Bytes table(bytes.begin() + at, bytes.begin() + at + size);
    at += size;
    if (i == 0)
      readTotkItemTable(next, table);
    else
      readTotkActorTable(next, table);
  }
  if (at != bytes.size())
    throw Error("Unexpected catalogue cache data.");
  data.variants = std::move(next.variants);
}
} // namespace
void readTotkCaption(const Bytes &bytes, Slot &slot) {
  totk::Save caption(bytes);
  auto number = [&](uint32_t hash) {
    auto &f = caption.field(hash);
    if (f.count != 1 || (f.type != totk::Type::Int && f.type != totk::Type::UInt))
      throw Error("Unexpected TOTK caption date field.");
    return caption.word(hash);
  };
  try {
    unsigned year = number(0x9811a3f7), month = number(0xdfd840d3), day = number(0xbd46f485),
             hour = number(0x23f3d75e), minute = number(0x27853bf7);
    if (year < 2000 || year > 2100 || month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 ||
        minute > 59)
      throw Error("Invalid TOTK caption date.");
    std::tm date{};
    date.tm_year = int(year) - 1900;
    date.tm_mon = int(month) - 1;
    date.tm_mday = int(day);
    date.tm_hour = int(hour);
    date.tm_min = int(minute);
    date.tm_isdst = -1;
    auto stamp = std::mktime(&date);
    if (stamp > 0) {
      slot.timestamp = uint64_t(stamp);
      char shown[80];
      if (std::strftime(shown, sizeof(shown), "%b %d, %Y · %I:%M %p", &date))
        slot.date = shown;
    }
  } catch (const Error &) {
  }
  auto automatic = caption.fields().find(0x25f03caa);
  if (automatic != caption.fields().end() && automatic->second.type == totk::Type::Bool) {
    slot.automatic = bytes[automatic->second.offset] != 0;
    slot.label = slot.automatic ? "Autosave" : "Manual save";
  }
  auto image = caption.fields().find(0x63696a32);
  if (image != caption.fields().end() && image->second.type == totk::Type::Binary) {
    auto &f = image->second;
    if (f.extent >= 8 && f.extent <= 4 * 1024 * 1024 && bytes[f.offset + 4] == 0xff &&
        bytes[f.offset + 5] == 0xd8)
      slot.image = Bytes(bytes.begin() + f.offset + 4, bytes.begin() + f.offset + f.extent);
  }
}
void loadTotkNames(Metadata &data, const std::string &path,
                   const std::function<void(const std::string &)> &log) {
  const auto cache = path + "/cache/totk";
  std::filesystem::create_directories(cache);
#ifdef __SWITCH__
  bool mounted = false, raw = false;
  FsFileSystem fs{};
  Result rc = fsOpenFileSystemWithPatch(&fs, TotkId, FsFileSystemType_ContentData);
  if (R_SUCCEEDED(rc)) {
    mounted = fsdevMountDevice("ztotk", fs) >= 0;
    if (!mounted)
      fsFsClose(&fs);
  }
  if (!mounted) {
    rc = romfsMountDataStorageFromProgram(TotkId, "ztotk");
    raw = mounted = R_SUCCEEDED(rc);
  }
  if (!mounted) {
    rc = romfsMountFromCurrentProcess("ztotk");
    raw = mounted = R_SUCCEEDED(rc);
  }
  if (mounted) {
    try {
      auto dictionaries = unpackSarc(unpackZstd(readFile("ztotk:/Pack/ZsDic.pack.zs")));
      bool found = false;
      for (auto &file : std::filesystem::directory_iterator("ztotk:/Mals")) {
        auto name = file.path().filename().string();
        if (name.rfind("USen.Product.", 0) != 0 || file.path().extension() != ".zs")
          continue;
        auto archive = unpackZstd(readFile(file.path().string()), dictionaries);
        namesFromArchive(data, archive);
        if (data.names.empty())
          throw Error("TOTK item name table was empty.");
        writeFile(cache + "/messages.sarc", archive);
        found = true;
        break;
      }
      if (!found)
        throw Error("TOTK English item names are unavailable.");
      std::vector<Bytes> decoded;
      for (auto prefix :
           {"PouchActorInfo.Product.", "GameActorInfo.Product.", "ActorInfo.Product."}) {
        std::vector<std::filesystem::path> tables;
        for (auto &file : std::filesystem::directory_iterator("ztotk:/RSDB")) {
          auto name = file.path().filename().string();
          if (name.rfind(prefix, 0) == 0 && file.path().extension() == ".zs")
            tables.push_back(file.path());
        }
        if (tables.size() != 1)
          throw Error("No unique TOTK item table was found.");
        decoded.push_back(unpackZstd(readFile(tables.front().string()), dictionaries));
      }
      auto bundle = catalogBundle(decoded);
      catalogFromBundle(data, bundle);
      writeFile(cache + "/catalog.bin", bundle);
      // Independent resources keep item browsing usable if one table is missing.
      for (auto spec :
           {std::pair<const char *, const char *>{"GameData/GameDataList.Product.", "enums.byml"},
            {"RSDB/Challenge.Product.", "challenge.byml"},
            {"RSDB/AttachmentActorInfo.Product.", "attachments.byml"}}) {
        try {
          std::string source = spec.first;
          auto slash = source.find('/');
          auto directory = "ztotk:/" + source.substr(0, slash);
          auto prefix = source.substr(slash + 1);
          std::vector<std::filesystem::path> files;
          for (auto &file : std::filesystem::directory_iterator(directory))
            if (file.path().filename().string().rfind(prefix, 0) == 0 &&
                file.path().extension() == ".zs")
              files.push_back(file.path());
          if (files.size() != 1)
            throw Error("No unique TOTK feature table was found.");
          auto bytes = unpackZstd(readFile(files.front().string()), dictionaries);
          if (std::string(spec.second) == "enums.byml")
            readTotkEnums(data, bytes);
          else if (std::string(spec.second) == "challenge.byml")
            readTotkChallenges(data, bytes);
          else
            readTotkAttachments(data, bytes);
          writeFile(cache + "/" + spec.second, bytes);
        } catch (const std::exception &e) {
          log(e.what());
        }
      }
      for (auto field : {"MainField", "MinusField"})
        for (auto kind : {"LocationArea", "HiddenKorok"}) {
          try {
            auto folder = std::string("ztotk:/Banc/") + field + "/" + kind;
            std::vector<std::filesystem::path> files;
            if (std::filesystem::is_directory(folder))
              for (auto &file : std::filesystem::directory_iterator(folder))
                if (file.path().extension() == ".zs")
                  files.push_back(file.path());
            if (files.empty())
              continue;
            if (files.size() != 1)
              throw Error("Ambiguous TOTK world table.");
            auto bytes = unpackZstd(readFile(files.front().string()), dictionaries);
            bool koroks = std::string(kind) == "HiddenKorok";
            readTotkWorld(data, bytes, koroks);
            writeFile(cache + "/" + field + "-" + kind + ".byml", bytes);
          } catch (const std::exception &e) {
            log(e.what());
          }
        }
      try {
        readBubbuls(data, cache + "/bubbuls.tsv");
      } catch (const std::exception &) {
        size_t count = 0;
        for (auto field : {"MainField", "MinusField"}) {
          auto directory = std::string("ztotk:/Banc/") + field;
          if (!std::filesystem::is_directory(directory))
            continue;
          for (auto it = std::filesystem::recursive_directory_iterator(directory);
               it != std::filesystem::recursive_directory_iterator(); ++it) {
            if (it->is_directory()) {
              if (it->path().filename() == "Merged")
                it.disable_recursion_pending();
              continue;
            }
            auto name = it->path().filename().string();
            if (name.size() < 14 || name.compare(name.size() - 14, 14, ".bcett.byml.zs"))
              continue;
            if (++count > 2000)
              throw Error("Unexpected world placement count.");
            readTotkPlacement(data, unpackZstd(readFile(it->path().string()), dictionaries));
          }
        }
        writeBubbuls(data, cache + "/bubbuls.tsv");
      }
    } catch (const std::exception &e) {
      log(e.what());
    }
    if (raw)
      romfsUnmount("ztotk");
    else
      fsdevUnmountDevice("ztotk");
  }
#endif
  if (data.names.empty())
    try {
      namesFromArchive(data, readFile(cache + "/messages.sarc"));
    } catch (const std::exception &e) {
      log(e.what());
    }
  if (data.variants.empty())
    try {
      catalogFromBundle(data, readFile(cache + "/catalog.bin"));
    } catch (const std::exception &e) {
      log(e.what());
    }
  if (data.enums.empty())
    try {
      readTotkEnums(data, readFile(cache + "/enums.byml", 32 * 1024 * 1024));
    } catch (const std::exception &e) {
      log(e.what());
    }
  if (data.quests.empty())
    try {
      readTotkChallenges(data, readFile(cache + "/challenge.byml"));
    } catch (const std::exception &e) {
      log(e.what());
    }
  if (data.attachments.empty())
    try {
      readTotkAttachments(data, readFile(cache + "/attachments.byml"));
    } catch (const std::exception &e) {
      log(e.what());
    }
  if (data.world.empty())
    for (auto field : {"MainField", "MinusField"})
      for (auto kind : {"LocationArea", "HiddenKorok"})
        try {
          readTotkWorld(data, readFile(cache + "/" + field + "-" + kind + ".byml"),
                        std::string(kind) == "HiddenKorok");
        } catch (const std::exception &e) {
          log(e.what());
        }
  if (std::none_of(data.world.begin(), data.world.end(),
                   [](auto &p) { return p.category == "Bubbulfrogs"; }))
    try {
      readBubbuls(data, cache + "/bubbuls.tsv");
    } catch (const std::exception &e) {
      log(e.what());
    }
  linkTotkVariants(data);
}
} // namespace zse
