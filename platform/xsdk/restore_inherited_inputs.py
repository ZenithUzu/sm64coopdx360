#!/usr/bin/env python3
"""Restores into the working tree the inherited files this fork stopped tracking, from upstream's own commit in its history.

The files stay out of Git, and no rights over them are granted here (docs/xbox360/private-inputs.json).
Existing files are kept. Run it from a full clone: a shallow one lacks the commit.
"""
import io
import json
import pathlib
import subprocess
import tarfile

UPSTREAM_COMMIT = "8cd6e5977"
BATCH = 200  # keeps each command under Windows' length limit


def main():
    root = pathlib.Path(__file__).resolve().parents[2]
    paths = json.loads((root / "docs/xbox360/private-inputs.json").read_text(encoding="utf-8"))["paths"]
    missing = [p for p in paths if not (root / p).exists()]
    for start in range(0, len(missing), BATCH):
        batch = missing[start:start + BATCH]
        data = subprocess.run(["git", "archive", "--format=tar", UPSTREAM_COMMIT, "--", *batch], cwd=root, stdout=subprocess.PIPE, check=True).stdout
        with tarfile.open(fileobj=io.BytesIO(data)) as tar:
            tar.extractall(root, filter="data")
    print(f"Restored {len(missing)} files; {len(paths) - len(missing)} were already present.")


if __name__ == "__main__":
    main()
