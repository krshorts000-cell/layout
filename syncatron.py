#!/usr/bin/env python3
"""
Sync-a-tron (Python Edition for Geometry Dash 2.2)
Based on HawKyre's Sync-a-tron (https://www.youtube.com/watch?v=t5QR2hArBJg)

Synchronizes rhythm data from osu! (.osu) beatmaps into Geometry Dash editor guidelines.
"""

import os
import sys
import zlib
import base64
import shutil

def parse_osu_timestamps(osu_path, offset_seconds=0.0):
    if not os.path.exists(osu_path):
        raise FileNotFoundError(f"File not found: {osu_path}")

    timestamps = []
    in_hit_objects = False

    with open(osu_path, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line == "[HitObjects]":
                in_hit_objects = True
                continue
            if in_hit_objects:
                if line.startswith("["):
                    break
                parts = line.split(",")
                if len(parts) >= 3:
                    try:
                        ms = float(parts[2])
                        sec = (ms / 1000.0) + offset_seconds
                        if sec >= 0:
                            timestamps.append(round(sec, 2))
                    except ValueError:
                        pass

    # Deduplicate and sort
    timestamps = sorted(list(set(timestamps)))
    return timestamps

def timestamps_to_guidelines(timestamps, color_code="0"):
    """
    Geometry Dash guideline format:
    time1~color1~time2~color2~...~timeN~colorN~
    color "0" is standard orange.
    """
    if not timestamps:
        return ""
    return "~".join(f"{t}~{color_code}" for t in timestamps) + "~"

def decode_save_file(dat_path):
    with open(dat_path, "rb") as f:
        raw = f.read()

    # Geometry Dash saves are XOR 11, URL-safe base64, zlib/gzip compressed
    # Or in some cases uncompressed XML if previously modified
    if raw.startswith(b"<?xml") or raw.startswith(b"<plist"):
        return raw.decode("utf-8", errors="ignore"), "xml"

    xored = bytes([b ^ 11 for b in raw])
    b64_str = xored.decode("latin1").replace("-", "+").replace("_", "/")
    # Pad base64 if needed
    missing_padding = len(b64_str) % 4
    if missing_padding:
        b64_str += "=" * (4 - missing_padding)

    compressed = base64.b64decode(b64_str)
    # Decompress gzip/zlib
    xml = zlib.decompress(compressed, 15 + 32).decode("utf-8", errors="ignore")
    return xml, "compressed"

def encode_save_file(xml_text):
    compressed = zlib.compress(xml_text.encode("utf-8"))
    b64 = base64.b64encode(compressed).decode("latin1").replace("+", "-").replace("/", "_")
    xored = bytes([ord(c) ^ 11 for c in b64])
    return xored

def decode_level_string(raw_level_data):
    """
    GD level data in <k>k4</k><s>...</s> is base64 + gzip
    """
    b64_str = raw_level_data.replace("-", "+").replace("_", "/")
    missing_padding = len(b64_str) % 4
    if missing_padding:
        b64_str += "=" * (4 - missing_padding)
    compressed = base64.b64decode(b64_str)
    try:
        decompressed = zlib.decompress(compressed, 15 + 32).decode("utf-8", errors="ignore")
    except Exception:
        decompressed = zlib.decompress(compressed).decode("utf-8", errors="ignore")
    return decompressed

def encode_level_string(level_xml_data):
    compressed = zlib.compress(level_xml_data.encode("utf-8"))
    b64 = base64.b64encode(compressed).decode("latin1").replace("+", "-").replace("/", "_")
    return b64

def inject_guidelines_to_first_level(dat_path, guideline_str):
    if not os.path.exists(dat_path):
        raise FileNotFoundError(f"CCLocalLevels.dat not found at: {dat_path}")

    # Backup
    backup_path = dat_path + ".bak"
    shutil.copy2(dat_path, backup_path)
    print(f"Created backup at: {backup_path}")

    xml, mode = decode_save_file(dat_path)

    # Locate first level data: between <k>k_0</k> and </d>
    if "<k>k_0</k>" not in xml:
        raise ValueError("No levels found in save file!")

    first_level_chunk = xml.split("<k>k_0</k>")[1].split("</d>")[0]
    if "<k>k4</k><s>" not in first_level_chunk:
        raise ValueError("First level has no object string (k4). Please place at least one object in the level and save it.")

    raw_level_string = first_level_chunk.split("<k>k4</k><s>")[1].split("</s>")[0]
    decompressed_level = decode_level_string(raw_level_string)

    # Inject kA14,<guidelines>
    parts = decompressed_level.split(";", 1)
    header = parts[0]
    objects = parts[1] if len(parts) > 1 else ""

    tokens = header.split(",")
    new_tokens = []
    i = 0
    found_ka14 = False
    while i < len(tokens):
        if tokens[i] == "kA14":
            new_tokens.append("kA14")
            new_tokens.append(guideline_str)
            found_ka14 = True
            i += 2
        else:
            new_tokens.append(tokens[i])
            i += 1

    if not found_ka14:
        new_tokens.append("kA14")
        new_tokens.append(guideline_str)

    new_header = ",".join(new_tokens)
    new_decompressed_level = new_header + ((";" + objects) if objects else "")

    new_level_string = encode_level_string(new_decompressed_level)
    new_xml = xml.replace(raw_level_string, new_level_string)

    new_dat_bytes = encode_save_file(new_xml)
    with open(dat_path, "wb") as f:
        f.write(new_dat_bytes)

    print(f"Successfully injected guidelines into first level of {dat_path}!")

def main():
    print("=" * 60)
    print("  Sync-a-tron 2.2 - Geometry Dash & osu! Beatmap Synchronizer")
    print("=" * 60)

    osu_file = None
    offset = 0.0

    if len(sys.argv) > 1:
        osu_file = sys.argv[1]
    if len(sys.argv) > 2:
        try:
            offset = float(sys.argv[2])
        except ValueError:
            pass

    if not osu_file:
        maps_dir = os.path.join(os.path.dirname(__file__), "osuMaps")
        os.makedirs(maps_dir, exist_ok=True)
        print(f"Drop your .osu beatmap file into: {maps_dir}")
        osu_files = [f for f in os.listdir(maps_dir) if f.endswith(".osu")]
        if osu_files:
            print("Available beatmaps:")
            for idx, fn in enumerate(osu_files, 1):
                print(f"  [{idx}] {fn}")
            choice = input(f"Choose map (1-{len(osu_files)}) or enter full path: ").strip()
            if choice.isdigit() and 1 <= int(choice) <= len(osu_files):
                osu_file = os.path.join(maps_dir, osu_files[int(choice) - 1])
            elif os.path.exists(choice):
                osu_file = choice
        else:
            osu_file = input("Enter path to .osu file: ").strip()

    if not osu_file or not os.path.exists(osu_file):
        print(f"Error: file not found '{osu_file}'")
        return

    print(f"\nParsing: {osu_file} (Offset: {offset}s)...")
    timestamps = parse_osu_timestamps(osu_file, offset)
    print(f"Found {len(timestamps)} rhythmic notes.")

    guidelines = timestamps_to_guidelines(timestamps)

    save_path = os.path.expandvars(r"%LOCALAPPDATA%\GeometryDash\CCLocalLevels.dat")
    if not os.path.exists(save_path):
        print(f"Error: CCLocalLevels.dat not found at {save_path}")
        return

    inject_guidelines_to_first_level(save_path, guidelines)
    print("\nDONE! Open Geometry Dash and enter your first level.")
    print("Guidelines are now perfectly synchronized with the osu! beatmap!")

if __name__ == "__main__":
    main()
