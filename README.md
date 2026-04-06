# GudaIO

Modular companion DLL for WoW 1.12.1 (TurtleWoW).

## How It Works

GudaIO is loaded by the TurtleWoW launcher via `dlls.txt` when the game starts. It runs **once**, during the game's startup sequence, before the login screen appears. After it finishes, it does nothing for the rest of the session - no background threads, no hooks, no memory patches. It simply reads and writes files on disk while the game is loading.

If you restart the game, it runs again. If you switch characters without restarting, it does not run again - the data from startup is used for the entire session.

If the [Guda](https://github.com/vati-io/Guda) addon is not installed, the DLL detects this and skips all work. It will not crash or produce errors.

## Setup

1. Copy `GudaIO.dll` into your TurtleWoW folder (where `WoW.exe` is)
2. Open `dlls.txt` in the same folder with a text editor
3. Add `GudaIO.dll` on a new line so it looks like:
   ```
   twdiscord.dll
   GudaIO.dll
   ```
4. Launch the game as usual

## Features

### Cross-Account Character Sharing
Requires the [Guda](https://github.com/vati-io/Guda) addon. Shares gold, bags, bank, mail, and equipped items between different WoW accounts on the same PC.

On each game launch, the DLL reads `WTF/Account/*/SavedVariables/Guda.lua` from every account folder, merges the character data, and writes the result to `Interface/AddOns/Guda/GudaShared.lua`. The addon picks this up automatically and shows other accounts' characters in the gold tooltip, inventory counts, and character dropdowns.

**First time:**
- Log into each account at least once and log out properly (don't kill the process) so that each account's character data gets saved
- Restart the game - the DLL will merge all accounts on the next launch

**After that:**
- Every time you start the game, all accounts see each other's latest data
- Characters can be hidden or removed from the right-click menu on the money frame

## Uninstall

1. Delete `GudaIO.dll` from the TurtleWoW folder
2. Remove the `GudaIO.dll` line from `dlls.txt`
