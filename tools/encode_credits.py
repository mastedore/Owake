#!/usr/bin/env python3
"""
    encode_credits.py

    Turns the credits text into the byte array used by src/state/info.cpp.
    Prints the C++ line ready to paste, or patches info.cpp with --apply.

    Copyright (c) 2025-2026 Mastedore <marcos@mastedore.com>

    This program is licensed under MIT license.
"""

import argparse
import sys

import credits_codec as cc


def main():
    ap = argparse.ArgumentParser(
        description="Encode the Owake credits text into the opcodes[] array of info.cpp.",
        epilog=cc.ESCAPE_HELP + "\n\nexample:\n  python3 tools/encode_credits.py 'The \\0\\1\\2\\3\\4\\nProject. 1.2.0\\pCopyright \\5 2026'",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    src = ap.add_mutually_exclusive_group(required=True)
    src.add_argument("text", nargs="?", help="escaped credits text (use single quotes in bash)")
    src.add_argument("-f", "--file", help="read the text from a file ('-' for stdin); real line breaks in it are ignored, use \\n and \\p")
    ap.add_argument("--apply", nargs="?", const=str(cc.DEFAULT_INFO_CPP), metavar="INFO_CPP",
                    help="replace the array in info.cpp instead of only printing it (default: src/state/info.cpp)")
    ap.add_argument("--preview", action="store_true", help="also draw how each page looks on the LCD")
    ap.add_argument("--cols", type=int, default=16, help="display columns for the checks (default 16)")
    ap.add_argument("--rows", type=int, default=2, help="display rows for the checks (default 2)")
    args = ap.parse_args()

    if args.file is not None:
        raw = sys.stdin.read() if args.file == "-" else open(args.file, encoding="ascii").read()
        # Lets a long message be split over several lines of the file.
        text = raw.replace("\r", "").replace("\n", "")
    else:
        text = args.text

    try:
        data = cc.encode(text)
    except cc.CreditsError as e:
        cc.die(str(e))

    for w in cc.check_layout(data, args.cols, args.rows):
        print(f"warning: {w}", file=sys.stderr)

    if args.preview:
        print(cc.preview(data, args.cols, args.rows), file=sys.stderr)
        print(file=sys.stderr)

    if args.apply:
        try:
            cc.apply_to_file(args.apply, data)
        except (cc.CreditsError, OSError) as e:
            cc.die(str(e))
        print(f"updated {args.apply} ({len(data) + 1} bytes)", file=sys.stderr)
    else:
        print(cc.to_cpp_line(data))


if __name__ == "__main__":
    main()
