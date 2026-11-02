# Licensing scope / alcance de las licencias

This repository is a derivative source tree, not a work covered by one blanket license.

## Original Xbox 360 port modules

The original implementation in the following newly introduced files is offered under the MIT license in [licenses/MIT-XBOX360.txt](licenses/MIT-XBOX360.txt):

- `platform/**` source, scripts, shaders, tests and documentation authored for this port;
- `include/level_script_read.h`;
- `src/pc/audio/audio_sdl3.c` and `.h`;
- `src/pc/controller/controller_xsdk.c` and `.h`;
- `src/pc/gfx/gfx_xsdk.c`, `.h`, `gfx_xenos.c` and `.h`;
- new Xbox 360 documentation and `tools/verify_public_tree.py`.

This grant covers only original contributions in those files, not included headers, libraries, inherited snippets, generated game data, trademarks or other third-party material. MIT does not override any applicable upstream terms.

## Inherited code and modifications

The sm64coopdx / sm64ex-coop / sm64-port / n64decomp portions retain their provenance and existing terms/notices. There was no single root license in the checked-out upstream snapshot. This fork does not represent that missing notices grant unrestricted reuse, nor claim authority to relicense inherited code or modifications to it under MIT. Obtain any permissions required from the relevant rights holders.

## Dependencies and excluded material

Lua, zlib and other third-party components retain their own notices: see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). ROMs, game assets, proprietary SDKs and reference executables are not granted a license by this repository and are excluded from the current publication tree. Earlier fork history is not rewritten by this change.

No whole-repository “MIT”, GPL or public-domain claim is made. This document clarifies scope; it does not certify legal compliance or guarantee that a rights holder will not make a claim.
