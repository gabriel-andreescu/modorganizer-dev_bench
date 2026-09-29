// SPDX-License-Identifier: MIT
#include "DevBenchAPI.h"

#include <Windows.h>

namespace DevBenchAPI {
IDevBenchInterface001* GetDevBenchInterface001() {
    // Windows headers define `interface` as a macro, so the cache uses another name.
    static IDevBenchInterface001* cached {};
    if (cached != nullptr) {
        return cached;
    }

    auto* const module = GetModuleHandleA(kModuleName);
    if (module == nullptr) {
        return nullptr;
    }

    const auto query = reinterpret_cast<QueryInterfaceFn>(GetProcAddress(module, kQueryInterfaceExport));
    cached = query != nullptr ? static_cast<IDevBenchInterface001*>(query(1)) : nullptr;
    return cached;
}
}
