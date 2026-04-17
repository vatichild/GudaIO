# GudaIO

Modular companion DLL for WoW 1.12.1.

## How It Works

GudaIO is loaded via `dlls.txt` when the game starts. It registers Lua functions that extend the WoW API with capabilities impossible through the addon system alone — such as client-side raid target icons visible at any range and per-unit GUID identification.

The DLL uses a background thread that periodically registers its Lua functions, ensuring they survive `/reload` without interfering with other DLLs (like SuperWoW). If the companion addons are not installed, the functions are still available for `/script` usage.

## Setup

1. Copy `GudaIO.dll` into your WoW 1.12.1 folder (where `WoW.exe` is)
2. Open `dlls.txt` in the same folder with a text editor
3. Add `GudaIO.dll` on a new line
4. Launch the game as usual

## Features

### Cross-Account Character Sharing

Requires the [Guda](https://github.com/vatichild/guda) addon. Shares gold, bags, bank, mail, and equipped items between different WoW accounts on the same PC.

On each game launch, the DLL reads `WTF/Account/*/SavedVariables/Guda.lua` from every account folder, merges the character data, and writes the result to `Interface/AddOns/Guda/GudaShared.lua`. The addon picks this up automatically and shows other accounts' characters in the gold tooltip, inventory counts, and character dropdowns.

**First time:**
- Log into each account at least once and log out properly (don't kill the process) so that each account's character data gets saved
- Restart the game — the DLL will merge all accounts on the next launch

**After that:**
- Every time you start the game, all accounts see each other's latest data
- Characters can be hidden or removed from the right-click menu on the money frame

### Solo Raid Target Icons

Works standalone or with [GudaPlates](https://github.com/vatichild/GudaPlates) for the full experience.

In vanilla WoW, raid target icons (skull, cross, star, etc.) can only be set by a party/raid leader. GudaIO removes this restriction by writing directly to the client's internal raid target table, making the 3D icons appear above mobs at any range — exactly like party/raid marks, but without needing a group.

#### How It Works Internally

1. **Raid Target Table** — The WoW client stores an array of 8 GUIDs (one per icon slot). When a GUID is in the table, the 3D engine renders the icon above that unit's head. Normally only the server writes to this table. GudaIO writes to it directly from the client side.

2. **GUID Identification** — The DLL calls the client's internal unit lookup function to resolve unit tokens (like `"target"`) to 64-bit GUIDs. This enables precise per-unit marking — no same-name confusion.

3. **Function Registration** — A background thread registers Lua functions every 3 seconds via `FrameScript_RegisterFunction`. This non-intrusive approach avoids hooking, so SuperWoW and other DLLs work alongside GudaIO without conflicts.

#### Registered Lua Functions

| Function | Description |
|----------|-------------|
| `GudaIO_SetRaidTarget(unit, index)` | Set a raid icon on a unit. `index`: 1=Star, 2=Circle, 3=Diamond, 4=Triangle, 5=Moon, 6=Square, 7=Cross, 8=Skull, 0=Clear |
| `GudaIO_GetRaidTarget(unit)` | Returns the current icon index (1-8) for a unit, or 0 if none |
| `GudaIO_UnitGUID(unit)` | Returns the unit's GUID as two numbers (hi, lo). Use `string.format("0x%08X%08X", hi, lo)` to format |
| `UnitGUID(unit)` | Same as `GudaIO_UnitGUID`. Provided as a standard API name if not already registered by SuperWoW |

#### Standalone Usage (without GudaPlates)

```lua
-- Mark your current target with skull
/script GudaIO_SetRaidTarget("target", 8)

-- Clear the mark
/script GudaIO_SetRaidTarget("target", 0)

-- Check if target has a mark
/script DEFAULT_CHAT_FRAME:AddMessage("Icon: " .. GudaIO_GetRaidTarget("target"))
```

#### With [GudaPlates](https://github.com/vatichild/GudaPlates)

GudaPlates provides the full UI integration for GudaIO's marking system:

**Solo Target Marking (no party required):**
- Right-click any NPC → **Raid Target Icon** submenu with all 8 colored icons
- Right-click a player → **Raid Target Icon** added to the context menu
- Keybinds available in **Key Bindings → GudaPlates** (bind any key to Set Star, Set Skull, etc.)
- Slash commands: `/gp mark 1` through `/gp mark 8`, `/gp mark 0` to clear, `/gp clearmarks` to clear all
- Pressing the same mark again toggles it off
- 3D icon above the mob visible at any range (rendered by the game engine)
- Nameplate icon on the health bar when in nameplate range
- Target frame icon when the marked unit is selected
- One unit per icon — reassigning an icon automatically clears the previous holder

**Mark as Tank:**
- Right-click a party/raid member → **Mark as Tank** / **Unmark Tank**
- Flagged players get a shield icon on their raid frame
- Mobs tanked by flagged players show **light blue** nameplates (OTHER_TANK threat color)
- Works even if the other player doesn't have GudaPlates installed
- Setting persists across `/reload`

**Graceful Fallback (without GudaIO):**
- GudaPlates detects if GudaIO is available and falls back gracefully
- Without the DLL: marks work on nameplates (name-based, same-named mobs may share marks) and on the target frame
- No 3D icons at range without the DLL — this is a client engine limitation that only the DLL can overcome
- Mark as Tank and all other features work normally without the DLL

## Compatibility

- **SuperWoW** — Fully compatible. GudaIO does not hook `FrameScript_RegisterFunction`, so SuperWoW's own function registrations are not affected. If SuperWoW provides `UnitGUID`, GudaIO will not overwrite it.
- **TurtleWoW** — Built and tested on TurtleWoW 1.12.1 client. Offsets are specific to this build.
- **Other DLLs** — GudaIO uses a non-intrusive polling approach with no memory patching or hooks, so it coexists safely with other `dlls.txt` entries.

## Building

Requires Visual Studio 2017 Build Tools (or later) and CMake.

```bash
cd build
cmake ..
cmake --build . --config Release
```

The output DLL is at `build/Release/GudaIO.dll`.

## Uninstall

1. Delete `GudaIO.dll` from the game folder
2. Remove the `GudaIO.dll` line from `dlls.txt`

## Disclaimer

This project is not affiliated with or endorsed by Blizzard Entertainment. World of Warcraft is a registered trademark of Blizzard Entertainment, Inc. This software is provided for use with private servers running client version 1.12.1.

## License

MIT License - see [LICENSE](LICENSE) for details.
