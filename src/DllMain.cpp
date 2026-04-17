#include <windows.h>
#include "AccountMerge.h"
#include "RaidTarget.h"

// GudaIO DLL - Companion DLL for WoW 1.12.1 (TurtleWoW)
// Loaded via dlls.txt by the TurtleWoW launcher.
// Runs modules at startup before the game initializes.
// Can support both addon-related features and standalone WoW improvements.

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        AccountMerge::Run();
        RaidTarget::Init();
    }
    return TRUE;
}
