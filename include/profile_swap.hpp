// Copyright (c) 2026 StockiestThree9
#pragma once
#include "save.hpp"
#include <filesystem>

namespace zse {
struct SwapInfo {
    int normal[2]{};
    bool pending=false;
};
// These functions operate on an already-mounted save. Only swap/continue require a writable mount.
SwapInfo inspectProfiles(const std::filesystem::path& root);
bool profileSwapPending(const std::filesystem::path& root);
std::string exchangeProfiles(const std::filesystem::path& root,const std::filesystem::path& backup,
    const std::function<void()>& commit,const std::function<void(size_t)>& checkpoint={});
std::string continueProfileSwap(const std::filesystem::path& root,const std::function<void()>& commit,
    const std::function<void(size_t)>& checkpoint={});
}
