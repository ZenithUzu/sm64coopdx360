# Third-party notices

Existing per-file notices and license files remain authoritative. This is an inventory, not a grant of rights over inherited material.

| Component | Provenance / terms |
| --- | --- |
| Game/decompilation and inherited port code | Nintendo / n64decomp / sm64-port / sm64ex-coop / Coop Deluxe and their contributors. Existing terms remain; no blanket relicense is claimed. See `credits.txt`. |
| Lua 5.3.6 source in `lib/xsdk/lua53` | Lua.org, PUC-Rio; MIT notice in `lua.h` and adjacent `LICENSE`. Imported from `lua/lua` commit `75ea9ccbea7c4886f30da147fb67b693b2624c26`. |
| zlib 1.3.1 source in `lib/xsdk/zlib` | Jean-loup Gailly and Mark Adler; zlib license in adjacent `LICENSE` and `zlib.h`. Imported from `madler/zlib` commit `51b7f2abdade71cd9bb0e7a373ef2610ec6f9daf`. |
| miniz | Notice retained in `src/pc/utils/miniz/LICENSE` and source. |
| SM64 tools | `tools/sm64tools.LICENSE`; existing author notices retained. |
| n64graphics CI tools | `tools/n64graphics_ci_dir/LICENSE`; retained. |
| IDO-related source/material | `tools/ido5.3_compiler/LICENSE.md`; retained, not relicensed. |
| stb and other embedded utilities | Their source-header notices remain; inspect each component before reuse. |
| xSDK, SDL3 Xenon, GCC toolchain | External build prerequisites with their own terms; SDK/toolchain binaries are not included. |
| Microsoft Xbox 360 SDK / XBDM | External tools/API prerequisite for optional diagnostics and mouse path; not redistributed and not licensed by this fork. |

XAM/XBDM interface ordinals were cross-checked against Xenia's public export tables. No extracted reference-XEX code or assets are shipped. References: https://github.com/xenia-project/xenia/blob/master/src/xenia/kernel/xam/xam_table.inc and https://github.com/xenia-project/xenia/blob/master/src/xenia/kernel/xbdm/xbdm_table.inc .

Text/media and mods inherited from upstream can have separate rights. Prebuilt libraries, executable tools, media and binary game/mod assets have been untracked for this source-only publication. They may still exist in upstream/earlier history or in a developer's private working directory. Removing them from the current tree is not a history purge.
