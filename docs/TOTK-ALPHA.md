# TOTK support — 0.1.5-alpha.4

An experimental game module in Zelda Save Editor, by StockiestThree9. The app now offers Breath of the Wild and Tears of the Kingdom from the same game picker.

## Available

- Discover TOTK's `slot_00` through `slot_05` under the selected console user; read `progress.sav` and optional `caption.sav` metadata and screenshots.
- Edit rupees, maximum/current hearts, stamina wheels, battery cells, equipment capacity and Pony Points.
- Edit existing weapons, bows, arrows, shields, armor, materials, food, key items and Zonai devices. Available values include quantities, durability, equipment bonus choices, bonus strength, dye color and meal values. Fuse durability controls appear only on fused equipment; items stored as ownership markers have no quantity control.
- Add, replace and remove ordinary inventory items. Duplicate weapons, bows, shields, armor and meals with their companion data intact. Special story weapons and key-item identities are protected. Material and device stacks cannot be duplicated; edit their quantity instead.
- Choose one armor item, then its upgrade level (0–4 stars where supported). Hylian Hood offers Hood up/Hood down independently of stars. Existing armor retains dye and equipment state when changing variants.
- Pick one weapon, then Decayed or Pristine where both exist. Installed appearance aliases, such as wooden sticks, share one entry with an Appearance choice. Existing weapon variants retain durability, bonuses and fusion.
- Filter the item catalogue with L/R: Head/Torso/Legs for armor; One-handed/Two-handed/Spears for weapons. Y searches names, options and IDs. Pick options before confirming Add or Replace; cancelling does not edit the save.
- Equip and unequip weapons, bows, shields and armor. Removing an item updates the equipped-slot references. Replacing armor is restricted to the same body part.
- Remove a weapon or shield's fused attachment while keeping the base item and its current durability. Selecting a replacement attachment is still future work.
- Fill all owned arrow, material or Zonai-device stacks to 999 in one confirmed, undoable edit.
- Choose named meal effects, including extra hearts, stamina recovery, gloom repair, resistance effects, attack/defense/speed, stealth and glow.
- Preserve TOTK's separate temporary challenge inventory. Item management checks array sizes and ordinary pouch limits; shrinking a pouch cannot hide existing items.
- Edit the three saved coordinates. These apply in the saved map; changing maps and teleport destinations are not implemented.
- Undo/redo, explicit confirmation, verified original slot backups, stale-document refusal and write verification, using the existing editor workflow. TOTK writes go to its own title and `progress.sav`, with game-labeled backup folders. Companion files are backed up and preserved.
- English item names are loaded from the installed game's message archive and cached locally. The NRO contains no game text, screenshots, sound samples, dictionaries or saves. If loading names fails, raw item IDs remain usable.

The catalogue uses installed item names and matching game flags. Upgrade chains, hood conversion, weapon model pairs and base durability are derived from the installed game's RSDB tables. The local `catalog.bin` cache contains those private game tables and is not distributed. The inspected 1.4.3 resources collapse 552 armor variants into 135 choices, and 115 weapon variants into 80 choices. Actual inventory copies remain separate.

New equipment starts at the installed game's base durability and opens its detail screen after creation. Without actor data it defaults to 1, explicitly labeled as unavailable game stats; reload Game resources to restore verified variant choices. Without names, the catalogue offers existing item IDs. Unverified or ambiguous variant relationships are kept separate. New stacks contain one item; new meals start with no effect and editable zero recovery values. Replacement keeps the existing slot's values and ingredients.

Stamina is displayed in wheels: 1 wheel = 1,000 game units. Battery capacity is displayed in cells: 1 cell = 1,000 game units, 3 cells per battery, up to 48 cells across both rows. Hearts use 4 game units each.

## First console check

Extract the package's `switch` directory onto the SD card. Open Sphaira in application mode by holding R while starting TOTK, then launch Zelda Save Editor. This also lets the resource loader fall back to the current title if mounting installed resources fails. Close the game before editing its save.

Choose Tears of the Kingdom. First verify that saves open, the displayed values match the game, and item names load. Settings → Game resources can retry reading names. Keep an independent save backup before testing a write. On the first write test, make one small rupee change, save, then verify it in-game.

The ordinary TOTK manual save and autosaves are supported. BOTW's Master Mode, transferred Profile 2 and swap controls belong only to BOTW. TOTK transferred secondary profiles have not been investigated in this build.

## Current limits

This remains an editor alpha. Key-item creation, bulk equipment repair, fusion replacement, horses, Autobuild import/export, quests, world completion, map visualization and coordinated completion actions are not implemented for TOTK. Caption thumbnails and metadata retain the game's last saved state until the game next saves.

The editor accepts known save revisions `0x46C3C8`, `0x47E0F4` and `0x49E946` after structural validation. Only `0x47E0F4` has been tested against a real progress file so far. Other revisions and actual Switch filesystem/gameplay behavior need device testing. Unknown revisions are refused.

## Validation and provenance

The local test fixture came from the public [progress.sav example in Marc Robledo's repository](https://github.com/marcrobledo/savegame-editors/blob/master/zelda-totk/progress.sav), downloaded 2026-10-04. SHA-256: `5de51b8d4aa730dab30137cfc3fc93a0bea9e564edbf27ab72321d35217fae13`. It stays in a private development directory and is not packaged or installed on the console.

The tests cover inventory insertion, replacement, duplication and compaction; ingredient/fuse/bonus preservation; equipped-reference repair; full-pouch and stale-item refusal; protected items; temporary-pouch isolation; exact undo/redo; meal choices; human-readable units; caption metadata; verified backups; neighboring-slot/BOTW isolation; and stale-save refusal. A synthetic `ActorMsg` archive checks the resource-loader regression. The alpha 3 checks additionally verify all ten hood variants, star/style independence, pristine/decayed mappings, initial durability, ambiguous chains, BYML v7/wide values, targeted identity-only writes, exact undo/redo, catalogue filters, pending-option cancellation and owned-item controls. Both games' interfaces are checked, with desktop images inspected for spacing and labels.

The user ran a read-only catalogue helper under installed TOTK 1.4.3 on 2026-10-06. Private copies of PouchActorInfo, GameActorInfo and ActorInfo and the installed English message archive were used to verify these relationships; no third-party item database or editor code was copied. The full application resource mount/cache path still needs console acceptance. Caption tests currently use a synthetic file. Desktop tests cannot establish that an edited save loads correctly in the game.

Build requires the existing Switch toolchain plus `switch-libzstd`. For local tests:

```sh
make test-totk FIXTURE=/private/progress.sav
# Optional installed 1.4.3 catalogue + messages under <root>/cache/totk:
make test-totk FIXTURE=/private/progress.sav CATALOG=/private/resource-root
# For UI tests, put a private fixture in host-data/totk/saves/slot_00/progress.sav
make host
./build/zelda-editor-preview.exe --totk-self-test
```
