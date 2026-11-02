# CoopDX xSDK + SDL3 integration

First actual game interface: src/pc/audio/audio_sdl3.c implements AudioAPI using
SDL3 streams. Engine input is 32 kHz signed native-endian 16-bit stereo; queue
counts stay in input frames while SDL converts to the Xenon output format.
The adapter acquires/releases one SDL audio subsystem reference, bounds the
queue, and destroys its stream on failure/shutdown.

Build the hardware harness with ./platform/xsdk-sdl3/build.ps1 from PowerShell.
Requires locally built xSDK and SDL-xenon; paths are script parameters.
Output: platform/xsdk-sdl3/build/coopdx-sdl3.xex.

Console: blue initially, A green and a quiet 440 Hz tone, B red, Back exits.
Gamepad discovery/hotplug uses SDL3. Audio is fed through the real game AudioAPI.
The log coopdx-sdl3.log records device initialization and queued audio blocks;
hearing sound and checking the visible frame require user observation.

Host compile/link/XEX packaging passed. Console audio verification is pending.
The game executable is NOT ported yet. The root PC build remains unchanged;
this adapter is not selected there until the complete Xbox target is ready.
SDL-xenon has no native video/GL renderer: the next graphics backend must use
xSDK GPU APIs. Other work remains: game input adapter, filesystem, SDL2 users,
Lua/DynOS dependencies, host asset generation, endian audit, and network tests.
