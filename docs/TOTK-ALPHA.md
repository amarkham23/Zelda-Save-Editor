# TOTK support — 0.1.5-alpha.5

An experimental game module in Zelda Save Editor, by StockiestThree9. The app now offers Breath of the Wild and Tears of the Kingdom from the same game picker.

## Available

- Discover TOTK's `slot_00` through `slot_05` under the selected console user; read `progress.sav` and optional `caption.sav` metadata and screenshots.
- Edit rupees, maximum/current hearts, stamina wheels, battery cells, equipment capacity and Pony Points. Player → Abilities changes both saved ownership flags for Ultrahand, Fuse, Ascend, Recall, Autobuild and amiibo.
- Edit existing weapons, bows, arrows, shields, armor, materials, food, key items and Zonai devices. Available values include quantities, durability, equipment bonus choices, bonus strength, dye color and meal values. Fuse durability controls appear only on fused equipment; items stored as ownership markers have no quantity control.
- Add, replace and remove ordinary inventory items, including collectible key stacks for Korok Seeds, Lights of Blessing, Bubbul Gems and Sage's Wills when verified by the installed catalogue. Duplicate weapons, bows, shields, armor and meals with their companion data intact. Special story weapons and story key-item identities are protected. Material, device and collectible key stacks cannot be duplicated; edit their quantity instead.
- Choose one armor item, then its upgrade level (0–4 stars where supported). Hylian Hood offers Hood up/Hood down independently of stars. Existing armor retains dye and equipment state when changing variants.
- Pick one weapon, then Decayed or Pristine where both exist. Installed appearance aliases, such as wooden sticks, share one entry with an Appearance choice. Existing weapon variants retain durability, bonuses and fusion.
- Filter the item catalogue with L/R: Head/Torso/Legs for armor; One-handed/Two-handed/Spears for weapons. Y searches names, options and IDs. Pick options before confirming Add or Replace; cancelling does not edit the save.
- Equip and unequip weapons, bows, shields and armor. Removing an item updates the equipped-slot references. Replacing armor is restricted to the same body part.
- Change or remove a weapon or shield's fused attachment. The searchable attachment list comes from the installed game. Replacement keeps base durability and the existing fuse bonus; it does not automatically grant a fresh fuse bonus. Remaining and maximum fuse bonus are separately editable.
- Repair owned weapons, bows or shields using verified base durability and durability modifiers. Upgrade owned armor to its highest verified rank, preserving hood position and dye; convert supported owned decayed weapons to their pristine version. Register decayed weapons as previously broken to enable pristine spawns without resetting existing world drops.
- Fill all owned arrow, material or Zonai-device stacks to 999 in one confirmed, undoable edit.
- Choose named meal effects, including extra hearts, stamina recovery, gloom repair, resistance effects, attack/defense/speed, stealth and glow.
- Preserve TOTK's separate temporary challenge inventory. Item management checks array sizes and ordinary pouch limits; shrinking a pouch cannot hide existing items.
- Edit the three saved coordinates. These apply in the saved map; changing maps and teleport destinations are not implemented.
- Edit existing registered horses: Unicode names, bond percentage, strength as health, named speed/pull stars, stamina spurs, mane, saddle and bridle. Horse creation, type replacement and body/color customization are not implemented.
- Browse saved discovery or completion flags for Koroks, shrines, lightroots, towers, Bubbulfrogs, caves, wells, chasms and named locations. Each category has individual choices and a confirmed bulk flag action. These actions do not grant rewards or reconcile story progress and sparse world-object records.
- Edit compendium states and unlock missing entries using the game's stock-picture state, preserving existing photographs. Edit direct quest stages with installed enum choices; related story and reward flags are separate.
- Edit map-stamp icon, map layer and coordinates, or clear all stamps. Colored pins remain unchanged. Reset saved amiibo touch dates while preserving character identities and lifetime scan counts.
- Browse all 30 Autobuild slots, manage up to eight favorites, and export/import the editor's checksummed `.zsbp` files. Imports keep the target slot's favorite setting and preserve other builds. Community blueprint formats are not supported.
- Inspect additional verified types in Advanced, including packed boolean arrays and UTF-16 names. Unmapped directly editable fields are labeled by their hash; unverified enums and sparse object-key structures are not offered as guessed choices.
- Undo/redo, explicit confirmation, verified original slot backups, stale-document refusal and write verification, using the existing editor workflow. TOTK writes go to its own title and `progress.sav`, with game-labeled backup folders. Companion files are backed up and preserved.
- English item and quest names, enum choices, attachment stats and world records are loaded from installed game resources and cached locally. The NRO contains no game text, screenshots, sound samples, dictionaries, map records or saves. If loading names fails, existing item IDs remain usable.

The catalogue uses installed item names and matching game flags. Upgrade chains, hood conversion, weapon model pairs and base durability are derived from the installed game's RSDB tables. The local `catalog.bin` cache contains those private game tables and is not distributed. The inspected 1.4.3 resources collapse 552 armor variants into 135 choices, and 115 weapon variants into 80 choices. Actual inventory copies remain separate.

New equipment starts at the installed game's base durability and opens its detail screen after creation. Without actor data it defaults to 1, explicitly labeled as unavailable game stats; reload Game resources to restore verified variant choices. Without names, the catalogue offers existing item IDs. Unverified or ambiguous variant relationships are kept separate. New stacks contain one item; new meals start with no effect and editable zero recovery values. Replacement keeps the existing slot's values and ingredients.

Stamina is displayed in wheels: 1 wheel = 1,000 game units. Battery capacity is displayed in cells: 1 cell = 1,000 game units, 3 cells per battery, up to 48 cells across both rows. Hearts use 4 game units each.

## First console check

Extract the package's `switch` directory onto the SD card. Open Sphaira in application mode by holding R while starting TOTK, then launch Zelda Save Editor. This also lets the resource loader fall back to the current title if mounting installed resources fails. Close the game before editing its save.

Choose Tears of the Kingdom. First verify that saves open, the displayed values match the game, and item names load. Settings → Game resources can retry reading names and feature tables. The first scan of active cave placements can take longer than later cached loads; no separate resource-check app is needed. Keep an independent save backup before testing a write. On the first write test, make one small rupee change, save, then verify it in-game.

The ordinary TOTK manual save and autosaves are supported. BOTW's Master Mode, transferred Profile 2 and swap controls belong only to BOTW. TOTK transferred secondary profiles have not been investigated in this build.

## Current limits

This remains an editor alpha, with further feature work remaining. Graphical map tools, community Autobuild format compatibility, complete horse creation/customization, sparse world-object completion (such as individual bosses, Addison signs and object-based collectibles) and coordinated quest/reward updates are not implemented. Direct stage or discovery edits are not a complete simulated in-game event. Caption thumbnails and metadata retain the game's last saved state until the game next saves.

World records are derived from the installed tables and included only when an editable field is present in the selected save. The inspected resources expose 900 Korok objectives (not the 1,000-seed reward total), 152 shrines, 120 lightroots and 147 active Bubbulfrog placements. Named cave/location flags can include game table entries that do not correspond one-to-one with a player's checklist. This release does not claim complete world-data parity.

Autobuild files are stored in `/switch/zelda-save-editor/blueprints/`. Export is a read-only SD operation; import changes the opened document until explicitly saved. Files have a checked version, exact payload size, finite camera values and checksum. The app does not decode or regenerate a build's internal object graph. Import files exported by this app; edited or unfamiliar blueprint payloads have not been validated in-game.

The editor accepts known save revisions `0x46C3C8`, `0x47E0F4` and `0x49E946` after structural validation. Only `0x47E0F4` has been tested against a real progress file so far. Other revisions and actual Switch filesystem/gameplay behavior need device testing. Unknown revisions are refused.

## Validation and provenance

The local test fixture came from the public [progress.sav example in Marc Robledo's repository](https://github.com/marcrobledo/savegame-editors/blob/master/zelda-totk/progress.sav), downloaded 2026-10-04. SHA-256: `5de51b8d4aa730dab30137cfc3fc93a0bea9e564edbf27ab72321d35217fae13`. It stays in a private development directory and is not packaged or installed on the console.

The tests cover inventory insertion, replacement, duplication and compaction; ingredient/fuse/bonus preservation; equipped-reference repair; full-pouch and stale-item refusal; protected items; temporary-pouch isolation; exact undo/redo; meal choices; human-readable units; caption metadata; verified backups; neighboring-slot/BOTW isolation; and stale-save refusal. A synthetic `ActorMsg` archive checks the resource-loader regression. Variant checks verify all ten hood variants, star/style independence, pristine/decayed mappings, initial durability, ambiguous chains, BYML v7/wide values, targeted identity-only writes, catalogue filters and pending-option cancellation.

Alpha 5 adds installed enum/quest/attachment/world-table checks, Unicode name round trips and invalid-input refusal, packed-bit neighbor invariance, Autobuild checksum/import/undo checks, equipment bulk actions, compendium/world states, paired ability ownership, map stamps, daily amiibo reset and collectible-key management. Both games' interfaces are checked, including 40 repeated world-list visits with a stable stack and byte-identical browsing. Desktop images are inspected for spacing and labels.

The user ran a read-only catalogue helper under installed TOTK 1.4.3 on 2026-10-06. Private copies of PouchActorInfo, GameActorInfo and ActorInfo and the installed English message archive were used to verify these relationships. Existing private resource captures additionally supplied Challenge, AttachmentActorInfo, GameDataList, location/Korok and active placement data for alpha 5; the new placement reader parsed all 950 supplied files successfully. No third-party item database or editor implementation was copied. Selected schema identifiers and horse stat encodings were checked against public format references and the actual fixture. The full application resource mount/cache path still needs console acceptance. Caption tests currently use a synthetic file. Desktop tests cannot establish that an edited save loads correctly in the game.

Build requires the existing Switch toolchain plus `switch-libzstd`. For local tests:

```sh
make test-totk FIXTURE=/private/progress.sav
# Optional installed 1.4.3 catalogue + messages under <root>/cache/totk:
make test-totk FIXTURE=/private/progress.sav CATALOG=/private/resource-root
# For UI tests, put a private fixture in host-data/totk/saves/slot_00/progress.sav
make host
./build/zelda-editor-preview.exe --totk-self-test
```
