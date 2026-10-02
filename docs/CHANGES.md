# Changes

## 0.1.4 beta 1

First public beta under the MIT license, credited to StockiestThree9. Includes the normal-only swap behavior and compact interface from 0.1.4 preview, installation-ready SD ZIP, full source package with libpulsar, dependency notices and SHA-256 checksums. Gameplay after swaps and wider device save-and-reload testing remain beta validation work.

## 0.1.4 preview

- Adopt the selected Quill & Scroll icon in Sphaira and throughout the app.
- Replace the large game tile with a compact list row and restrict touch selection to that row.
- Limit Profile 1 / Profile 2 swapping to normal slots 0–5, their save thumbnails and normal Hero's Path files. Master Mode saves and history remain at their original locations.
- Remove Master Mode from the swap menu and hide its swap shortcut on the Master Mode tab.
- Preserve shared albums, compendium pictures, game options and other tracker files in place. Explain the resulting picture-library limitation before confirming a swap.
- Verify protected data during recovery and completion. Refuse unfinished 0.1.3 whole-profile journals and direct recovery to that version.
- Pass 230 swap checks, covering all ten rename interruption points in the fixture, protected-data invariance, two-way progress retention and full backups. Storage and interface checks also pass.

## 0.1.3 preview

- Save selection opens on normal Profile 1 saves. L/R and touch switch between Normal, Profile 2 and Master Mode; unfamiliar layouts receive an additional Other tab.
- Profile 2 explicitly explains that the Switch 2 save location is inactive on this Switch.
- Y opens a review screen for exchanging complete game profiles. Both sets of saves, Master Mode, albums, compendium images and Hero's Path are retained; global game options stay unchanged.
- Every exchange makes and verifies a complete SD backup, uses checked directory moves, and records progress inside the save container. Interrupted operations can be resumed; editing is blocked while recovery is pending.
- Swapping back uses the current data, retaining progress made in either adventure between swaps.
- Added 195 swap checks including failures after every one of 16 directory moves, exact round trips, new-progress retention, backup failure and conflict refusal.

## 0.1.2 preview

- Skip user selection on consoles with one account; Back returns directly to the game picker.
- Replace the loading screen with centered text and an animated native-style indicator. File work runs separately from drawing, with exit deferred until the operation completes.
- Find nested transferred saves, including the six Profile 2 slots observed in the user's `ch_01` folder.
- Show Profile 1 / Profile 2 in save selection, the editor header, loading, confirmation and backup information. Preserve the original folder when editing.
- Identify Master Mode only for its known slot numbers, rather than treating every additional folder as Master Mode.
- Add OLED black alongside Automatic, Light and Dark, with persistent preferences.
- Verify discovery, profile-specific writes, backup identity, unsafe path rejection and user-picker navigation in host tests.

## 0.1.1 preview

- Stamina is shown and edited in wheels, including partial wheels. Conversion to the game's internal units is automatic.
- Updating permanent stamina preserves any existing temporary bonus and updates the current capacity.
- Health and meal healing use hearts. Horse bond uses a percentage. Food prices show rupees.
- Equipment lists explicitly label durability. Stackable items use a quantity marker; armor does not display a misleading quantity.
- Save selection is now a native-style list with an original-size preview, location, date, manual/autosave information and newest-first sorting.
- Thumbnails retain their original proportions and use smooth scaling when reduced.
- Equipment modifiers have named choices. Equipped state is kept consistent within each equipment section.
- Footer hints only advertise controls available on the current screen.

## 0.1.0 preview

Initial independent C++/libnx application with BOTW support, typed resource readers, native UI, console-sourced sounds, undo/redo and verified backups. Author credit: StockiestThree9.
