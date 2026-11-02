# Xbox 360 bring-up

This is an initial platform smoke test, NOT a playable sm64coopdx port.
It links no game code and needs no ROM. Requires a locally installed Xbox 360 SDK
and a console able to run the resulting XEX (not a stock retail console).

## Build

From PowerShell:

    .\platform\xbox360\build.ps1

Set XEDK to your SDK installation if it is not already defined. Output defaults to
platform/xbox360/build/default.xex; use -OutDir to choose another directory.
Do not commit generated binaries or redistribute SDK files.

## Hardware check

Launch default.xex on the console. Expected: blue screen at 720p; hold A on
controller slot 0 for green, B for red (B takes priority). Release for blue.
Use your dashboard's normal exit mechanism. Hardware behavior is unverified.

## Port status

Only this standalone compile/link/XEX path is implemented. The root Makefile
still targets PC and does not build Xbox 360. The smoke test is intentionally
separate until the platform and compiler compatibility work is ready.

Next milestones:
1. Validate display and input on actual hardware.
2. Audit MSVC PowerPC compatibility: GNU attributes/extensions, C dialect,
   pointer width, alignment, and big-endian assumptions in data and networking.
3. Add Xbox implementations of GfxWindowManagerAPI and GfxRenderingAPI;
   the PC D3D11 and SDL2 backends cannot be selected unchanged.
4. Port filesystem/timing/threading, audio, and controller interfaces.
5. Build required dependencies for Xbox; gate Discord, updater, and unsupported
   services explicitly, preserving a path to LAN multiplayer.
6. Integrate asset generation on the host and game compilation for the target,
   then validate boot, gameplay, Lua mods, and coop synchronization.

Local build validation is not evidence of game compatibility or console boot.
