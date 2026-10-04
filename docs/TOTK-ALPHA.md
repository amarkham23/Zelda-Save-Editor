# TOTK support — 0.1.5-alpha.1

An experimental game module in Zelda Save Editor, by StockiestThree9. The app now offers Breath of the Wild and Tears of the Kingdom from the same game picker.

## Available

- Discover TOTK's `slot_00` through `slot_05` under the selected console user; read `progress.sav` and optional `caption.sav` metadata and screenshots.
- Edit rupees, maximum/current hearts, stamina wheels, battery cells, equipment capacity and Pony Points.
- Edit existing weapons, bows, arrows, shields, armor, materials, food, key items and Zonai devices. Available values include quantities, durability, equipment bonus choices, bonus strength, dye color and meal values. Fuse durability controls appear only on fused equipment; items stored as ownership markers have no quantity control.
- Edit the three saved coordinates. These apply in the saved map; changing maps and teleport destinations are not implemented.
- Undo/redo, explicit confirmation, verified original slot backups, stale-document refusal and write verification, using the existing editor workflow. TOTK writes go to its own title and `progress.sav`, with game-labeled backup folders. Companion files are backed up and preserved.
- English item names are loaded from the installed game's message archive and cached locally. The NRO contains no game text, screenshots, sound samples, dictionaries or saves. If loading names fails, raw item IDs remain usable.

Stamina is displayed in wheels: 1 wheel = 1,000 game units. Battery capacity is displayed in cells: 1 cell = 1,000 game units, 3 cells per battery, up to 48 cells across both rows. Hearts use 4 game units each.

## First console check

Extract the package's `switch` directory onto the SD card. Open Sphaira in application mode by holding R while starting TOTK, then launch Zelda Save Editor. This also lets the resource loader fall back to the current title if mounting installed resources fails. Close the game before editing its save.

Choose Tears of the Kingdom. First verify that saves open, the displayed values match the game, and item names load. Settings → Game resources can retry reading names. Keep an independent save backup before testing a write. On the first write test, make one small rupee change, save, then verify it in-game.

The ordinary TOTK manual save and autosaves are supported. BOTW's Master Mode, transferred Profile 2 and swap controls belong only to BOTW. TOTK transferred secondary profiles have not been investigated in this build.

## Current limits

This is an initial editor alpha, not full BOTW feature parity. Item addition/removal/replacement, fusion replacement, horses, quests, world completion, map visualization and coordinated completion actions are not implemented for TOTK. Caption thumbnails and metadata retain the game's last saved state until the game next saves.

The editor accepts known save revisions `0x46C3C8`, `0x47E0F4` and `0x49E946` after structural validation. Only `0x47E0F4` has been tested against a real progress file so far. Other revisions and actual Switch filesystem/gameplay behavior need device testing. Unknown revisions are refused.

## Validation and provenance

The local test fixture came from the public [progress.sav example in Marc Robledo's repository](https://github.com/marcrobledo/savegame-editors/blob/master/zelda-totk/progress.sav), downloaded 2026-10-04. SHA-256: `5de51b8d4aa730dab30137cfc3fc93a0bea9e564edbf27ab72321d35217fae13`. It stays in a private development directory and is not packaged or installed on the console.

192 checks pass against that file and isolated host storage fixtures. They cover human-readable units, existing items across all nine categories, enum edits, only targeted bytes changing, byte-identical undo and redo, rollback, bounds validation, caption metadata, verified backups, neighboring-slot and BOTW isolation, and stale-save refusal. The shared BOTW core, storage, 230 profile-swap checks and both games' UI navigation checks also pass. Desktop UI images were inspected for spacing and labels.

The runtime resource reader and caption decoder are implemented, but installed TOTK resources and a real caption have not yet been checked on the user's Switch. Caption tests currently use a synthetic file. Desktop tests cannot establish that an edited save loads correctly in the game.

Build requires the existing Switch toolchain plus `switch-libzstd`. For local tests:

```sh
make test-totk FIXTURE=/private/progress.sav
# For UI tests, put a private fixture in host-data/totk/saves/slot_00/progress.sav
make host
./build/zelda-editor-preview.exe --totk-self-test
```
