#!/usr/bin/env python3
"""
    decode_credits.py

    Reads the opcodes[] array from src/state/info.cpp (or any C++ line / list
    of bytes) and prints it back as escaped text, in the same format that
    encode_credits.py takes, so edit -> encode round-trips cleanly.

    Copyright (c) 2025-2026 Mastedore <marcos@mastedore.com>

    This program is licensed under MIT license.
"""

import argparse
import sys
from pathlib import Path

import credits_codec as cc


def main():
    ap = argparse.ArgumentParser(
        description="Decode the Owake credits array back into readable, escaped text.",
        epilog=cc.ESCAPE_HELP + "\n\nexamples:\n  python3 tools/decode_credits.py\n"
               "  python3 tools/decode_credits.py --line 'static const char opcodes[] PROGMEM = {0x54, 0x68, 0x0c};'",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    src = ap.add_mutually_exclusive_group()
    src.add_argument("source", nargs="?", help="file with the array ('-' for stdin), default src/state/info.cpp")
    src.add_argument("-l", "--line", help="the C++ line or the bytes themselves, passed directly")
    ap.add_argument("--preview", action="store_true", help="also draw how each page looks on the LCD")
    ap.add_argument("--cols", type=int, default=16, help="display columns for the preview (default 16)")
    ap.add_argument("--rows", type=int, default=2, help="display rows for the preview (default 2)")
    args = ap.parse_args()

    try:
        if args.line is not None:
            source = args.line
        elif args.source == "-":
            source = sys.stdin.read()
        else:
            source = Path(args.source or cc.DEFAULT_INFO_CPP).read_text(encoding="ascii")
        data = cc.parse_array(source)
    except (cc.CreditsError, OSError) as e:
        cc.die(str(e))

    if cc.STOP not in data:
        print("warning: no 0x0c terminator, typeEffect() would run off the end of this array", file=sys.stderr)

    if args.preview:
        print(cc.preview(data, args.cols, args.rows), file=sys.stderr)
        print(file=sys.stderr)

    print(cc.decode(data))


if __name__ == "__main__":
    main()
