#pragma once

namespace RaidTarget {

// Spawns a background thread that registers Lua functions:
// GudaIO_SetRaidTarget, GudaIO_GetRaidTarget, GudaIO_UnitGUID, UnitGUID.
// Re-registers every 3s to survive /reload. Called from DllMain.
void Init(void* hModule);

} // namespace RaidTarget
