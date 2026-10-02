# Validation record

## Native build

- Switch NRO produced with devkitA64 r30 and libnx 4.12.0.
- The user confirmed the 0.1.0 native build opens and its system navigation sounds work on their OLED Switch.
- The user also reported that 0.1.1's interface looked good. Version 0.1.2 adds single-user selection, animated loading, Profile 2 discovery and OLED black. A new device save-and-load acceptance test is still required before a public release.

## Save and metadata tests

- 43 deterministic checks run without private data: both byte orders, CRC32 reference, exact no-op preservation, numeric/string edits, undo/redo, rollback, malformed input and bounds.
- 128 checks pass with the user's private 1.9.0 save and extracted resources. All 43,672 fields match the independently parsed dictionary. The game supplies 45,026 usable definitions and 1,690 item names.
- Real-save tests cover rupees, durability conversion, 1.2 stamina wheels mapping to 1,200 game units, matching stamina capacity, equipment state, inventory addition/removal, record order, values and byte-identical undo.
- The user's original backup is never written by the tests.

## Storage tests

On synthetic files, the platform storage path verifies exact backups, backs up companion files, verifies written bytes, accepts successful writes, refuses stale documents and preserves edits after a refused write. This does not substitute for a console filesystem acceptance test.

A read-only directory inspection on the user's console found eight root slots and six additional slots under `ch_01/0` through `ch_01/5`. Their game files are 1,027,248 bytes. A retrieved Profile 2 caption uses the supported `0x4730` format. The full Profile 2 game document was not copied for host testing. Synthetic fixtures matching the observed directory layout check both profiles, Master Mode labels, newest-first sorting, and an unfamiliar layout. Editing a nested Profile 2 slot leaves both Profile 1 and neighboring Profile 2 slots byte-identical. Backups retain the original folder identity. Invalid paths are refused. OLED preferences survive reload.

## Interface checks

The desktop build uses the same drawing and interaction code. Automated checks cover settings before selecting a save, single-user bypass and Back behavior, multiple-user selection, loading error propagation, player navigation, inventory tabs, detail navigation and cancellation as the default for confirmations. Navigation leaves all save bytes unchanged.

Light, dark and OLED screenshots were inspected for layout. The save picker uses the original 256 × 144 screenshot for its preview, preserves the aspect ratio of list thumbnails, reads caption metadata and sorts by timestamp. A synthetic fourteen-slot layout exercises scrolling, Profile 2 badges and Master Mode labels without requiring more private game files. Screenshots are captured before presenting the frame so they do not read an invalidated backbuffer.

## 0.1.3 profile exchange

- The 0.1.2 device log confirmed all fourteen real save slots were discovered, including six nested Profile 2 slots. Caption metadata identifies the user's actual manual/auto positions; numerical fallback labels are used only without usable captions.
- 195 synthetic swap checks pass. Every one of sixteen directory-move interruption points is resumed and reversed, recovering every original byte and directory. Checks include unequal slot counts, absent folders, Master Mode, shared assets, global options, empty first profiles and new progress/photos created between swaps.
- Backup failure leaves live data unchanged. An unfamiliar layout is refused. A conflicting file introduced during a simulated interruption is retained and recovery refuses to overwrite it.
- Existing storage tests pass with the pending-swap guards. UI tests cover the default Normal filter, L/R tab switching, filtered save identity, selection reset and safe confirmation defaults.
- Normal, Profile 2, Master Mode and swap-confirmation screenshots were inspected. The new swap has not been run on the user's live saves, and actual gameplay after a swap has not yet been verified.

## Distribution boundary

The NRO's RomFS contains only the selected quill-and-scroll logo. The source archive includes the generated source artwork and its prompt. Private `.sav`, `.pack`, `.bfsar`, host cache files and screenshots of the user's game are excluded from the source/release archives. System audio, font and game dictionaries are obtained locally at runtime.

## 0.1.4 normal-save exchange and interface

- Switch and desktop builds succeed. 230 normal-swap checks pass, including all ten rename interruption points in the fixture and successful recovery and reversal after each.
- Master Mode saves, captions, hard-mode history, shared albums, compendium pictures, unrelated tracker data and global options remain byte-identical throughout swaps and recovery. An unsupported Master Mode document does not block a normal-save exchange.
- Normal Hero's Path exchanges correctly when file counts differ. New normal progress and history travel with the adventure when swapping back; new shared photos stay in place. Full backups preserve every original file.
- An unfinished 0.1.3 whole-profile journal is refused without changing save data. Conflict refusal, backup failure, unfamiliar-layout refusal and empty-first-profile reversal remain covered.
- Storage tests and UI checks pass. The Master Mode tab ignores the swap button, swap rows contain no Master Mode entries, and touching blank space on the game picker cannot open BOTW.
- Light-mode game selection, OLED Master Mode, swap review and confirmation screenshots were inspected. The selected quill-and-scroll icon appears at both small header and list sizes.
- No swap was run on the user's live saves. Device gameplay after a normal-save exchange remains unverified.
