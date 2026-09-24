# PSP button icons

`button_icons.png` holds every button prompt the PSP build shows: in the
controls menu, next to the menu hints (accept, back, delete) and in the
in-game prompts. Drawn by [Stann](https://github.com/ThatStann).

The N64 build uses `assets/images/button_icons.png` and is not affected by
anything here.

## The sheet

- **64 x 64 pixels**, a 4 x 4 grid of **16 x 16 cells**, one icon per cell.
- **White on black.** The sheet is read as brightness only: white is the icon,
  black is transparent, greys are soft edges. The menu tints it (white, grey
  for unselected rows), so colour would be lost anyway.
- The game cuts out the whole cell, except the player numbers, which are
  **8 x 12** each.

| Row | Cell 1 | Cell 2 | Cell 3 | Cell 4 |
|-----|--------|--------|--------|--------|
| 1 | Triangle | Square | Circle | Cross |
| 2 | Face buttons (as a group) | D-pad (as a group) | Analog nub | Player 1, player 2 |
| 3 | D-pad up | D-pad left | D-pad down | D-pad right |
| 4 | START | SELECT | L | R |

The groups are what a *direction* shows when it is bound to them: moving or
looking can be put on the face buttons, the D-pad or the nub, and the controls
menu then shows that group's icon.

## Which icon shows where

The rectangles are in `src/menu/controls.c`, under `#ifdef PSP`. The PSP's
buttons stand in for the N64's as `src/system/psp/controller_psp.c` maps
them: the face buttons are the N64's C buttons, the D-pad is its D-pad, SELECT
is Z, and L, R and START are themselves. Cross and circle also accept and go
back in the menus.

If you move an icon to another place on the sheet, change its rectangle in
`controls.c` to match. If you only redraw it in place, nothing else changes.

## After editing

Rebuild the PSP target as usual. The PSP materials are generated from a copy
of `assets/materials/ui.skm.yaml` that points at this sheet (see
`assets/materials/psp/CMakeLists.txt`), so the new icons are picked up with no
other step.
