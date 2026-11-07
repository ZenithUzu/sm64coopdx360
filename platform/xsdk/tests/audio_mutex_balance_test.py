#!/usr/bin/env python3
"""Fails if a function returns or ends holding gAudioThread's mutex: the audio worker would then wait forever."""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[3]
FILES = ("src/audio/data.c", "src/audio/external.c", "src/audio/heap.c", "src/audio/playback.c", "src/audio/seqplayer.c", "src/game/sound_init.c")


def main():
    errors = []
    for name in FILES:
        held = 0
        for number, line in enumerate((ROOT / name).read_text(encoding="utf-8", errors="ignore").split("\n"), 1):
            if "MUTEX_LOCK(gAudioThread)" in line:
                held += 1
            if held and re.search(r"\breturn\b", line) and "MUTEX_UNLOCK(gAudioThread)" not in line:
                errors.append(f"{name}:{number}: returns holding the mutex")
            if "MUTEX_UNLOCK(gAudioThread)" in line and not re.search(r"\breturn\b", line) and held:
                held -= 1
            if line.startswith("}"):
                if held:
                    errors.append(f"{name}:{number}: function ends holding the mutex")
                held = 0
    print("\n".join(errors) or "audio_mutex_balance_test: ok")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
