#include "RaidTarget.h"

#include <windows.h>
#include <cstdint>
#include <cstring>

// ============================================================================
// WoW 1.12.1 (TurtleWoW) - Client-side Raid Target Icons
// ============================================================================

// --- Types ---
typedef void (__fastcall *FRegisterFunc_t)(const char* name, void* func);
typedef const char* (__thiscall *FGetStringArg_t)(uintptr_t L);
typedef void (__thiscall *FGetNumberArg_t)(uintptr_t L);
typedef void (__thiscall *FPushNumber_t)(uintptr_t L);
typedef int  (__thiscall *FCheckArgs_t)(uintptr_t L);

// --- TurtleWoW offsets ---
static const uintptr_t ADDR_REGISTER  = 0x00704120;
static const uintptr_t ADDR_UNIT_GUID = 0x00515970;
static FGetStringArg_t pGetString     = (FGetStringArg_t)0x006F3690;
static FGetNumberArg_t pGetNumber     = (FGetNumberArg_t)0x006F3619;
static FPushNumber_t   pPushNumber    = (FPushNumber_t)  0x006F380E;
static FCheckArgs_t    pCheckArgs     = (FCheckArgs_t)   0x006F3510;
static uint64_t*       pRaidTargets   = (uint64_t*)0x00B71368;

// --- State ---
static const char s_setName[]  = "GudaIO_SetRaidTarget";
static const char s_getName[]  = "GudaIO_GetRaidTarget";
static const char s_guidName[] = "GudaIO_UnitGUID";
static const char s_unitGuid[] = "UnitGUID";
static DWORD s_mainThreadId    = 0;

// --- Lua helpers ---
static const char* GetStringArg(uintptr_t L, int idx) {
    const char* r;
    __asm {
        mov ecx, L
        mov edx, idx
        call pGetString
        mov r, eax
    }
    return r;
}
static double GetNumberArg(uintptr_t L, int idx) {
    double r;
    __asm {
        mov ecx, L
        mov edx, idx
        call pGetNumber
        fstp r
    }
    return r;
}
static void PushNumber(uintptr_t L, double v) {
    __asm {
        sub esp, 8
        fld v
        fstp qword ptr [esp]
        mov ecx, L
        call pPushNumber
    }
}
static int CheckArgs(uintptr_t L, int n) {
    int r;
    __asm {
        mov ecx, L
        mov edx, n
        call pCheckArgs
        mov r, eax
    }
    return r;
}
static uint64_t GetUnitGUID(const char* token) {
    uint32_t lo, hi;
    uintptr_t addr = ADDR_UNIT_GUID;
    __asm {
        mov ecx, token
        call addr
        mov lo, eax
        mov hi, edx
    }
    return ((uint64_t)hi << 32) | lo;
}

// ============================================================================
// Lua C functions
// ============================================================================

static int __fastcall LuaSetRaidTarget(uintptr_t L, uintptr_t) {
    if (!CheckArgs(L, 2)) return 0;
    const char* token = GetStringArg(L, 1);
    if (!token) return 0;
    int icon = (int)GetNumberArg(L, 2);
    uint64_t guid = GetUnitGUID(token);
    if (guid == 0) return 0;

    for (int i = 0; i < 8; i++)
        if (pRaidTargets[i] == guid) pRaidTargets[i] = 0;
    if (icon >= 1 && icon <= 8)
        pRaidTargets[icon - 1] = guid;

    return 0;
}

static int __fastcall LuaGetRaidTarget(uintptr_t L, uintptr_t) {
    if (!CheckArgs(L, 1)) { PushNumber(L, 0); return 1; }
    const char* token = GetStringArg(L, 1);
    if (!token) { PushNumber(L, 0); return 1; }
    uint64_t guid = GetUnitGUID(token);
    int result = 0;
    if (guid != 0)
        for (int i = 0; i < 8; i++)
            if (pRaidTargets[i] == guid) { result = i + 1; break; }
    PushNumber(L, (double)result);
    return 1;
}

static int __fastcall LuaUnitGUID(uintptr_t L, uintptr_t) {
    if (!CheckArgs(L, 1)) return 0;
    const char* token = GetStringArg(L, 1);
    if (!token) return 0;
    uint64_t guid = GetUnitGUID(token);
    if (guid == 0) return 0;
    PushNumber(L, (double)(uint32_t)(guid >> 32));
    PushNumber(L, (double)(uint32_t)(guid & 0xFFFFFFFF));
    return 2;
}

// ============================================================================
// Registration — suspends main thread for safety
// ============================================================================

static void RegisterLuaFunction(const char* name, void* func) {
    __asm {
        mov ecx, name
        mov edx, func
        call ADDR_REGISTER
    }
}

// ============================================================================
// Background thread — waits for Lua init, then polls
// ============================================================================

static DWORD WINAPI InitThread(LPVOID) {
    Sleep(15000);

    while (true) {
        __try {
            RegisterLuaFunction(s_setName,  (void*)LuaSetRaidTarget);
            RegisterLuaFunction(s_getName,  (void*)LuaGetRaidTarget);
            RegisterLuaFunction(s_guidName, (void*)LuaUnitGUID);
            RegisterLuaFunction(s_unitGuid, (void*)LuaUnitGUID);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            // Lua state invalid — retry next cycle
        }
        Sleep(5000);  // 5s interval reduces crash probability
    }

    return 0;
}

// ============================================================================
// Public API — called from DllMain on the MAIN thread
// ============================================================================

void RaidTarget::Init(void* hModule) {
    // DllMain runs on the main thread — capture its ID now
    s_mainThreadId = GetCurrentThreadId();

    HANDLE h = CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
    if (h) CloseHandle(h);
}
