"""Host-only generated files for the xSDK game build; run after extract_assets.py us."""
import argparse
import pathlib
import subprocess
import zlib
import sys
import concurrent.futures
import shutil

p = argparse.ArgumentParser()
p.add_argument("--repo", type=pathlib.Path, required=True)
p.add_argument("--out", type=pathlib.Path, required=True)
p.add_argument("--compiler", required=True)
a = p.parse_args()
repo, out = a.repo.resolve(), a.out.resolve()
out.mkdir(parents=True, exist_ok=True)
def write_changed(path, data):
    if isinstance(data, str):
        data = data.encode()
    if not path.exists() or path.read_bytes() != data:
        path.write_bytes(data)
# The repo's precompressed audio is little-endian and has 64-bit pointers.
# Reassemble from the user's extracted samples for Xenon's big-endian 32-bit ABI.
sample_root = out / "sound" / "samples"
sample_root.mkdir(parents=True, exist_ok=True)
samples = {x.relative_to(repo / "sound" / "samples"): x
           for x in (repo / "sound" / "samples").rglob("*.aiff")}
for folder in ("instruments", "bowser_organ", "course_start", "piranha_music_box"):
    for source in (repo / "sound" / "samples" / folder).glob("*.aiff"):
        samples.setdefault(pathlib.Path("extended") / source.name, source)
def encode_sample(item):
    relative, source = item
    dest = (sample_root / relative).with_suffix(".aifc")
    if dest.exists() and dest.stat().st_mtime_ns >= source.stat().st_mtime_ns:
        return
    dest.parent.mkdir(parents=True, exist_ok=True)
    codebook = dest.with_suffix(".table")
    with codebook.open("wb") as stream:
        subprocess.run([str(repo / "tools" / "aiff_extract_codebook.exe"), str(source)],
                       cwd=repo, stdout=stream, check=True)
    subprocess.run([str(repo / "tools" / "vadpcm_enc.exe"), "-c", str(codebook), str(source), str(dest)],
                   cwd=repo, check=True, stdout=subprocess.DEVNULL)
with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool:
    list(pool.map(encode_sample, samples.items()))
audio = out / "sound"
subprocess.run([sys.executable, "tools/assemble_sound.py", "--endian", "big", "--bitwidth", "32",
    "--output-dir", str(out), "-DVERSION_US=1", str(sample_root), "sound/sound_banks", str(audio / "sound_data.ctl"),
    str(audio / "ctl_header"), str(audio / "sound_data.tbl"), str(audio / "tbl_header")], cwd=repo, check=True)
sequences = [str(x) for x in sorted((repo / "sound" / "sequences" / "us").glob("*.m64"))]
prefix = pathlib.Path(a.compiler).name.removesuffix("gcc.exe")
tools = pathlib.Path(a.compiler).parent
for source in sorted((repo / "sound" / "sequences").glob("*.s")):
    obj = audio / (source.stem + ".o")
    binary = audio / "sequences" / "us" / (source.stem + ".m64")
    binary.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([str(tools / (prefix + "as.exe")), "--defsym", "VERSION_US=1",
        "--defsym", "BITS_32=1", "-I", str(repo / "include"), "-o", str(obj), str(source)], cwd=repo, check=True)
    subprocess.run([str(tools / (prefix + "objcopy.exe")), "-j", ".rodata", "-O", "binary", str(obj), str(binary)],
        cwd=repo, check=True)
    sequences.append(str(binary))
subprocess.run([sys.executable, "tools/assemble_sound.py", "--endian", "big", "--bitwidth", "32",
    "--output-dir", str(out), "-DVERSION_US=1", "--sequences", str(audio / "sequences.bin"), str(audio / "sequences_header"),
    str(audio / "bank_sets"), "sound/sound_banks", "sound/sequences.json", *sequences], cwd=repo, check=True)
for source, name in (("sound_data_compressed.ctl", "sound_data.ctl"),
                     ("sound_data_compressed.tbl", "sound_data.tbl"),
                     ("sequences_compressed.bin", "sequences.bin"),
                     ("bank_sets_compressed", "bank_sets")):
    data = zlib.decompress((audio / source).read_bytes())
    write_changed(audio / (name + ".inc.c"), ",".join(f"0x{x:02x}" for x in data) + ",\n")
for script, name, args in (
    ("mario_anims_converter.py", "mario_anim_data.c", []),
    ("demo_data_converter.py", "demo_data.c", ["assets/demo_data.json", "-DVERSION_US=1"])):
    dest = out / "assets" / name
    dest.parent.mkdir(parents=True, exist_ok=True)
    generated = subprocess.run([sys.executable, str(repo / "tools" / script), *args],
                       cwd=repo, capture_output=True, check=True)
    write_changed(dest, generated.stdout)
for png in sorted(repo.rglob("*.png")):
    if ".git" in png.parts or "build" in png.parts:
        continue
    fmt = png.stem.rsplit(".", 1)[-1]
    if fmt not in {"rgba32", "rgba16", "ia16", "ia8", "ia4", "ia1", "i8", "i4"}:
        continue
    dest = out / png.relative_to(repo).with_suffix(".inc.c")
    dest.parent.mkdir(parents=True, exist_ok=True)
    if dest.exists() and dest.stat().st_mtime_ns >= max(png.stat().st_mtime_ns,
            (repo / "tools" / "n64graphics.exe").stat().st_mtime_ns):
        continue
    subprocess.run([str(repo / "tools" / "n64graphics.exe"), "-s", "u8", "-i", str(dest),
                    "-g", str(png), "-f", fmt], cwd=repo, check=True,
                   stdout=subprocess.DEVNULL)
headers = out / "include" / "level_headers.h"
headers.parent.mkdir(parents=True, exist_ok=True)
preprocessed = subprocess.run([a.compiler, "-E", "-P", "-DVERSION_US=1", "-DNON_MATCHING=1",
    "-DAVOID_UB=1", "-I.", "-Iinclude", "-x", "c", "levels/level_headers.h.in"],
    cwd=repo, check=True, capture_output=True)
generated = subprocess.run([sys.executable, "tools/output_level_headers.py"], cwd=repo,
                   input=preprocessed.stdout, capture_output=True, check=True)
write_changed(headers, generated.stdout)
for name in ("define_text.inc.c", "define_courses.inc.c"):
    dest = out / "text" / "us" / name
    dest.parent.mkdir(parents=True, exist_ok=True)
    text = subprocess.run([a.compiler, "-E", "-P", "-DVERSION_US=1", "-DNON_MATCHING=1",
        "-DAVOID_UB=1", "-D_LANGUAGE_C", "-I.", "-Iinclude", "-Itext/us", "-x", "c", "text/"+name],
        cwd=repo, check=True, capture_output=True)
    intermediate = out / (name + ".preprocessed")
    intermediate.write_bytes(text.stdout)
    temporary = dest.with_suffix(dest.suffix + ".tmp")
    subprocess.run([str(repo / "tools" / "textconv.exe"), "charmap.txt", str(intermediate), str(temporary)],
                   cwd=repo, check=True)
    write_changed(dest, temporary.read_bytes())
    temporary.unlink()
(out / "assets.stamp").write_text("US xSDK generated assets\n")
print("Generated local xSDK game assets in", out)
