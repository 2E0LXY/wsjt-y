# JTTY (imported from WSJT-X)

Source: WSJT-X v3.2.0-rc1 (commit 567ad29), https://github.com/WSJTX/wsjtx
Authors: Joe Taylor K1JT, Steve Franke K9AN, Rob Sherwood G4KLA and the
WSJT-X development group. Licence: GPLv3.

Status: PORT IN PROGRESS on branch `feature/jtty`. Not for release.
The WSJT-X authors ask derivative works not to publish features before
they reach a General Availability release; WSJT-Y will not tag or ship
JTTY until WSJT-X 3.2.0 GA.

Stage 1 (this import): Fortran DSP / codec library only, compiled into
wsjt_fort. No mode menu entry, decoder hook or UI yet.
Not built (stage 2+ or standalone tools): sjtty.f90, sjtty_qrm.f90,
rjtty.f90, jtty.f90, update.f90, transmit.f90, jttyaudio.c, c_funcs.f90,
jtty_codewords.f90 (unreferenced upstream fragment).
