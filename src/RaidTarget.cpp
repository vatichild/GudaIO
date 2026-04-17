#include "RaidTarget.h"

#include <windows.h>
#include <cstdint>
#include <cstring>

// ============================================================================
// WoW 1.12.1 (TurtleWoW) - Client-side Raid Target Icons
// ============================================================================
//
// Hooks FrameScript_CreateEvents (the same function SuperWoW hooks) to
// register Lua functions on the MAIN THREAD during Lua initialization.
// Fires at startup and after /reload — no background thread, no crashes.
// ============================================================================

// --- Types ---
typedef const char* (__thiscall *FGetStringArg_t)(uintptr_t L);
typedef void (__thiscall *FGetNumberArg_t)(uintptr_t L);
typedef void (__thiscall *FPushNumber_t)(uintptr_t L);
typedef int  (__thiscall *FCheckArgs_t)(uintptr_t L);

// --- TurtleWoW offsets ---
static const uintptr_t ADDR_REGISTER       = 0x00704120;
static const uintptr_t ADDR_UNIT_GUID      = 0x00515970;
static const uintptr_t ADDR_CREATE_EVENTS  = 0x0051A550;
static FGetStringArg_t pGetString          = (FGetStringArg_t)0x006F3690;
static FGetNumberArg_t pGetNumber          = (FGetNumberArg_t)0x006F3619;
static FPushNumber_t   pPushNumber         = (FPushNumber_t)  0x006F380E;
static FCheckArgs_t    pCheckArgs          = (FCheckArgs_t)   0x006F3510;
static uint64_t*       pRaidTargets        = (uint64_t*)0x00B71368;

// --- Function names ---
static const char s_setName[]  = "GudaIO_SetRaidTarget";
static const char s_getName[]  = "GudaIO_GetRaidTarget";
static const char s_guidName[] = "GudaIO_UnitGUID";
static const char s_unitGuid[] = "UnitGUID";

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
// Registration helper
// ============================================================================

static void RegisterLuaFunction(const char* name, void* func) {
    __asm {
        mov ecx, name
        mov edx, func
        call ADDR_REGISTER
    }
}

static void RegisterAllFunctions() {
    RegisterLuaFunction(s_setName,  (void*)LuaSetRaidTarget);
    RegisterLuaFunction(s_getName,  (void*)LuaGetRaidTarget);
    RegisterLuaFunction(s_guidName, (void*)LuaUnitGUID);
    RegisterLuaFunction(s_unitGuid, (void*)LuaUnitGUID);
}

// ============================================================================
// Hook on FrameScript_CreateEvents — runs on MAIN THREAD
// ============================================================================

// Trampoline: saved bytes + jump back to original+N
static uint8_t s_trampoline[16];
static uint8_t s_savedBytes[8];

// Function pointer to the trampoline (cast to callable)
typedef void (__cdecl *TrampolineFunc_t)();
static TrampolineFunc_t pTrampoline = (TrampolineFunc_t)(void*)s_trampoline;

static void __declspec(naked) HookedCreateEvents() {
    __asm {
        // Call the original via trampoline function pointer
        call pTrampoline

        // Register our Lua functions (main thread, safe!)
        pushad
    }

    RegisterAllFunctions();

    __asm {
        popad
        ret
    }
}

static void InstallHook() {
    // Save original bytes at FrameScript_CreateEvents
    memcpy(s_savedBytes, (void*)ADDR_CREATE_EVENTS, 8);

    // Build trampoline: execute saved bytes, then jump back
    // Original prologue: 55 8b ec 83 ec 08 (6 bytes)
    // We overwrite 5 bytes with JMP, but need 6 in trampoline for complete instructions

    // Check if the first byte is E9 (already hooked by SuperWoW)
    uint8_t firstByte = s_savedBytes[0];
    int savedSize;

    if (firstByte == 0xE9) {
        // Already hooked (SuperWoW's JMP) — save 5 bytes, fix up relative offset
        savedSize = 5;
        memcpy(s_trampoline, s_savedBytes, 5);
        // Fix relative JMP offset: original target = ADDR + 5 + rel32
        int32_t origRel = *(int32_t*)&s_savedBytes[1];
        uintptr_t origTarget = ADDR_CREATE_EVENTS + 5 + origRel;
        int32_t newRel = (int32_t)(origTarget - ((uintptr_t)&s_trampoline[0] + 5));
        *(int32_t*)&s_trampoline[1] = newRel;
    } else {
        // Original prologue — save 6 bytes (complete instructions)
        savedSize = 6;
        memcpy(s_trampoline, s_savedBytes, 6);
    }

    // Add JMP back to original + savedSize
    s_trampoline[savedSize] = 0xE9;
    uintptr_t jumpBack = ADDR_CREATE_EVENTS + savedSize;
    int32_t backRel = (int32_t)(jumpBack - ((uintptr_t)&s_trampoline[savedSize] + 5));
    *(int32_t*)&s_trampoline[savedSize + 1] = backRel;

    // Make trampoline executable
    DWORD oldProt;
    VirtualProtect(s_trampoline, sizeof(s_trampoline), PAGE_EXECUTE_READWRITE, &oldProt);

    // Patch FrameScript_CreateEvents with JMP to our hook
    VirtualProtect((void*)ADDR_CREATE_EVENTS, 8, PAGE_EXECUTE_READWRITE, &oldProt);
    uint8_t* target = (uint8_t*)ADDR_CREATE_EVENTS;
    target[0] = 0xE9;
    int32_t hookRel = (int32_t)((uintptr_t)&HookedCreateEvents - (ADDR_CREATE_EVENTS + 5));
    *(int32_t*)&target[1] = hookRel;
    VirtualProtect((void*)ADDR_CREATE_EVENTS, 8, oldProt, &oldProt);
}

// ============================================================================
// Public API
// ============================================================================

void RaidTarget::Init(void* hModule) {
    InstallHook();
}
