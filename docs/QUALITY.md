# Quality pass — 0.1.5-alpha.4

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
