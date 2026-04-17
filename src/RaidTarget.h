#pragma once

namespace RaidTarget {

// Spawns a background thread that waits for the game's Lua engine
// to initialize, then registers GudaIO_SetRaidTarget(unit, index).
// Must be called from DllMain (DLL_PROCESS_ATTACH).
void Init();

} // namespace RaidTarget
