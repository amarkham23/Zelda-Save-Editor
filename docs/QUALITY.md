# Quality pass — 0.1.5-alpha.5

Reviewed on 2026-10-07 while expanding TOTK features. No live console saves were changed during development or installation.

## Alpha 5 review

- Typed writes preserve offsets, counts, neighboring packed bits and unrelated fields. UTF-16 names reject invalid UTF-8 and oversized input before writing; fixed-size binary payloads cannot resize the save. Undo/redo restores the exact fixture bytes.
- Fusion preserves base durability and existing bonus values. Equipment bulk actions use installed stats; protected story weapon identities are excluded from pristine conversion. Collectible key stacks reject duplicates while story keys remain protected.
- World completion uses existing verified fields and enum choices. Bubbulfrogs are selected from active placements rather than the dictionary's unused cave flags. Direct quest/world actions explicitly describe that rewards and related progress are separate.
- Autobuild imports validate file version, exact size, checksum and finite camera values. Favorites are limited to eight; empty-slot favorite/export actions are refused. Same-slot import is byte-identical and other build slots are preserved.
- The six-row save menu groups the new features. Both game walkthroughs passed; 40 repeated world-list visits kept a stable navigation stack and browsing left save bytes unchanged. Representative menu, horse, fusion, quest and blueprint images were inspected for labels, spacing and overlaps.

| Check | Result |
| --- | --- |
| TOTK real fixture and installed 1.4.3 resources | 7,090 checks passed |
| BOTW/core with private save and game resources | 580 checks passed |
| Synthetic/core | 495 checks passed |
| Storage | Backup, conflict, isolation, path and settings checks passed |
| Profile exchange | 230 checks passed, including interruption recovery and protected-file invariance |
| Active placement replay | 950 files parsed, zero failures, 147 unique Bubbulfrog placements |
| Host UI | Both game walkthroughs passed |
| Switch build | Successful, no compiler warnings or errors |

These checks use a real TOTK revision `0x47E0F4` fixture and resources from the user's installed 1.4.3 game. They do not establish gameplay acceptance of the new edits, compatibility of every revision, or complete resource-mount behavior on the console. The app still requires explicit confirmation and a verified original backup for a save write. Remaining feature limits are in [TOTK-ALPHA.md](TOTK-ALPHA.md).

## Previous alpha 4 review

Reviewed on 2026-10-06 after the user reported that alpha 3 was working well.

## Fixed findings

- **BOTW inventory limits:** capacity edits could hide owned equipment. The editor now counts owned equipment before accepting a lower capacity. Current hearts cannot exceed the stored maximum.
- **Stale inventory actions:** Undo/Redo could leave BOTW pickers with an earlier item identity. Both games return to the inventory after history changes. BOTW model operations additionally check the selected ID, index, group and ordinal before mutation.
- **Armor replacement:** BOTW now requires the replacement to use the same body part. The picker filters accordingly, preserving equipped-slot consistency.
- **Edit journal failure:** edits are restored if journal construction throws. Redo history is reserved before discarding it. Nested Undo/Redo/accept operations are refused.
- **Repeated large scans:** unsaved-state and change-count rendering no longer compares the entire save on each redraw. Counts are maintained by successful edits and history patches. Endian-aware synthetic tests compare the cached count with the actual changed bytes, including history truncation, failure, branch replacement and string edits.
- **Obsolete UI paths:** removed the unreachable save-card renderer; preserved the current save list. Paragraph wrapping now honors intentional line breaks. Desktop UI checks live under `tests/`, and `VERSION` supplies both app and NACP versions.

## Review scope and checks

Reviewed both save readers, transactions, numeric constraints, inventory mutations, resource/variant loading, profile discovery, backup/commit behavior and UI navigation. Original game data, caches and fixtures remain private. No live saves were changed by this work.

| Check | Result |
| --- | --- |
| BOTW/core with private save and game resources | 580 checks passed |
| TOTK real fixture and installed 1.4.3 catalogue | 1,999 checks passed |
| Storage | Backup, conflict, isolation, path and settings checks passed |
| Profile exchange | 230 checks passed, including interruption recovery and protected-file invariance |
| Host UI | Both game walkthroughs passed, including stale BOTW picker regression |
| Switch build | Successful |

The user tested alpha 3's item options successfully. Alpha 4's new constraints and real save-and-load behavior still need device acceptance. This is a targeted review and regression pass, not proof that every save revision or resource table is supported. Existing revision restrictions, transaction backups and unknown-layout refusals remain in force.
