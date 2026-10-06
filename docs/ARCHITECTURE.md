# Architecture and provenance

## Ownership boundaries

- `source/`, `include/`, `tests/`: new application, file-format readers, platform integration, interface and tests written for StockiestThree9.
- `artwork/quill-scroll.png`, `romfs/logo.jpg`, `icon.jpg`: selected original generated project artwork, with provenance in `artwork/README.md`.
- `third_party/libpulsar`: separately credited MIT dependency, unmodified.
- `host-data/`: private development fixtures. Excluded from Git and packages.

The existing personal fork was preserved in its original directory and was not used as the source tree for this rebuild. This is an independent implementation; it is not a formal legal clean-room certification.

## Layers

`save.cpp` owns BOTW's byte-preserving document and edit journal. The format is a header, repeated hash/value words, and a trailing marker. A transaction may change existing value bytes only. Failed transactions restore their snapshot, including failures while building the edit journal. Change counts are updated after transactions and while undoing/redoing patches; redraws do not scan the save. A no-op load does not reserialize or reorder the input.

`resources.cpp` contains bounded SARC, Yaz0, BYML and MSBT readers. It derives field types and item labels from installed game resources. Synthetic dictionary entries whose hashes do not match their names are skipped. No third-party editor tables are compiled into the application.

`botw.cpp` applies game-specific interpretation: quarter-heart units, whole durability points, inventory groupings, modifier arrays, food companion arrays, horses and typed field access. Item removal and addition move related arrays together inside one undoable transaction.

`platform.cpp` owns Switch services, profiles, save mounting, software keyboard, system sound loading, preferences and verified backups. Saves are mounted read-only for browsing. A writable mount exists only during an explicit save operation. The live file must still match the originally opened bytes before writing. A verified SD backup is required; a write failure triggers an attempted restoration and reports the backup path.

`ui.cpp` owns the HOS-style screen stack and widgets. It uses typed fields and the BOTW/TOTK adapters. Host walkthroughs are included from `tests/ui_walkthroughs.inc`; they are excluded from Switch builds. Navigation actions only move focus. List geometry and typography are fixed. Large advanced lists are rebuilt only after data or screen changes.

Blocking file operations use a single joined SDL worker, while all rendering, font access and applet event polling remain on the main thread. The UI does not inspect the changing document or resource tables until the worker finishes. Exceptions are passed back to the normal error dialog. Exit waits for any active operation, including a save, to complete.

Save discovery keeps the exact relative folder in each slot. The observed original BOTW slots are at the root, and Switch 2's retained second profile lives under `ch_01`. Discovery is bounded and also exposes unfamiliar nested `game_data.sav` files without guessing their profile. Open and commit validate the relative location, and backup names encode separators to avoid collisions between profiles. Directory-layout facts came from a read-only inspection of the user's own save container.

BOTW is the first module. Adding another Zelda game requires a format reader and game adapter, title metadata and a platform save binding. The generic rendering, settings, keyboard, backup and profile layers are reusable.

`profile_swap.cpp` owns the portable normal-save exchange transaction. It makes an exact full-container backup, serializes an immutable bounded move plan, exchanges normal directories 0–5 and numeric `trackblock` history files without overwriting destinations, commits completion markers, and resumes interrupted moves after verifying their retained contents. Master slots 6 and 7, hard-mode history, shared picture libraries, options and other files stay in place; their signatures are checked before recovery and after completion. Version 2 journals explicitly record the normal-only scope; unfinished wider version 1 journals are refused. The platform supplies the mount and filesystem commit; host tests supply fault injection. UI profile filters retain the exact selected slot identity, and thumbnail caches are invalidated after exchanging normal profiles.

`totk_inventory.cpp` manages the ordinary half of TOTK's fixed pouch arrays. Typed columns describe each slot's names, quantities, equipment/fusion values and five recipe ingredients. Creation, duplication and compaction journal all companion columns together. Equipped indices for the ordinary pouch are remapped after deletion; the temporary pouch's data and references are untouched. Mutations validate column types and dimensions first. Item catalogue candidates come from installed message labels with corresponding IsGet fields, plus the opened save's existing items. Special story weapons and key-item identities remain protected. The catalogue contains no imported third-party item database.

`totk_variants.cpp` groups catalogue entries using the installed PouchActorInfo upgrade/conversion links, ActorInfo model relationships and GameActorInfo durability values. It uses explicit rank links, not numeric actor-ID arithmetic. Ambiguous links are not exposed as interchangeable choices. Names resolve duplicate internal armor parents only when one parent uniquely matches. Hylian Hood conversion preserves rank; pristine models are paired with their exact, unique decay counterpart within the same model project. Cosmetic aliases require identical model, category, performance and durability. Item family choices are separate from owned inventory copies.

`totk_resources.cpp` decodes the three installed RSDB tables with the game's Zstandard dictionaries, caches a bounded length-prefixed `ZCA1` bundle and applies it only after all three tables parse. BYML v2–7 and checked 64-bit values are supported by the shared reader. Resources, item names and caches remain private. The options screen holds an uncommitted candidate until Add/Replace; existing-item variant edits journal only the item name, keeping every companion field unchanged.

## Audio

The app mounts system program `0100000000001000`, opens `/sound/qlaunch.bfsar`, and requests HOME menu focus, decide, finish and error cues through libpulsar. Sounds live in memory and are released on exit. If unavailable, the interface remains usable with audio off. Nintendo audio is neither copied into the release nor embedded in RomFS.

## Remaining release work

The public beta still needs a device save-and-load acceptance test and broader fixtures spanning completed games, Master Mode and different game versions. World editing is field-based; graphical map tools and coordinated bulk completion are future features. Original application code and artwork use MIT; preserve all linked dependency notices in binary distributions.
