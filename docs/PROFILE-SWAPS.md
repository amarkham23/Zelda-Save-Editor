# Normal-save swaps

Profile 1 is the original game's save location at the root of the save container. Profile 2 is the second location introduced by the Switch 2 edition, under `ch_01`. These labels describe where data is currently stored. They do not track which console originally created an adventure.

## Use

1. Close BOTW and open the editor.
2. Choose BOTW. The save picker defaults to **Normal**, showing normal Profile 1 saves.
3. Use **L/R** or touch the tabs to view Profile 2 or Master Mode. Master Mode includes both profiles and labels each location.
4. On the Normal or Profile 2 tab, press **Y** to review the two normal adventures. Choose the swap row and explicitly confirm **Swap saves**. The Master Mode tab has no swap action.
5. Keep the app open until verification completes. The formerly inactive set is now in the Profile 1 location used by the game.
6. Use the same action again to exchange the current sets. Any progress made between exchanges travels with its adventure.

The move changes storage locations without altering game fields or converting save versions. Host tests verify retention and recovery; gameplay compatibility after an exchange still needs a device acceptance test with the installed BOTW version. This is an unofficial operation, distinct from Nintendo's built-in transfer support.

## Retention

The exchange includes normal numbered folders **0–5**, preserving their game data, captions, thumbnails and any companion files. It also exchanges normal Hero's Path files named `tracker/trackblock<digits>.sav`. Swapping back uses the current contents, including new progress and normal history written between swaps.

**Master Mode is excluded.** Folders 6 and 7 and all `trackblock_hard` history files stay in their original locations. The swap menu lists only normal-save counts. The engine checks that data outside the normal swap has not changed before resuming and after completing an operation.

Shared `album` and `pict_book` libraries stay in their original locations to preserve Master Mode's pictures. Consequently, a swapped normal adventure may show the album and compendium pictures already stored at that location. Its individual save thumbnails do travel with it. Global `option.sav`, other tracker files and desktop metadata also stay where they were. All of these files are included in the full backup; the editor does not discard or overwrite either picture library. An unfamiliar layout is refused before any save data moves.

The app copies the complete save container to a unique SD backup directory and compares every copied file with its original. It then verifies the source and backup trees again. No existing backup is overwritten. File-content signatures in the recovery journal use file sizes and CRC32 to detect accidental changes; they are not cryptographic authentication.

Swapping uses directory and file renames into initially empty locations. It does not overwrite the destination's save files with the other profile. Data moves through temporary storage inside the same save container, then into its new profile. The temporary storage and journal live under `zse-profile-swap`.

## Interruption recovery

The immutable move plan is committed before any profile data moves. Each completed move receives a marker and a save-filesystem commit. A move interrupted before its marker is recognized by checking the retained source/destination and its expected contents. Conflicting data causes recovery to stop rather than overwrite it.

On reopening the editor, a pending operation offers **Resume interrupted swap**. Normal save opening and writing stay blocked until the exchange is verified. Both profiles and the full SD backup remain available. If recovery reports a damaged record or conflicting files, preserve the save container and backup for manual recovery; do not start BOTW or remove the staging directory.

On success, the journal is marked complete. Its empty staging directory and small completed record remain until the next swap. The next operation removes only this completed metadata before making a fresh full backup. Full SD backups are never automatically deleted.

Version 0.1.4 uses the `ZSE-NORMAL-SWAP-2` journal. It refuses an unfinished `ZSE-PROFILE-SWAP-1` operation from 0.1.3, which used a wider scope. Recover such an operation with 0.1.3 first; its files and full backup are retained. A completed 0.1.3 journal can be cleaned normally before starting a new normal-save swap.
