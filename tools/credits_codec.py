"""
    credits_codec.py

    Shared helpers for encode_credits.py and decode_credits.py.
    Converts between the escaped text people type and the byte array that
    src/state/info.cpp feeds to typeEffect().

    Copyright (c) 2025-2026 Mastedore <marcos@mastedore.com>

    This program is licensed under MIT license.
"""

import re
import sys
from pathlib import Path

# Control bytes understood by typeEffect() in src/state/info.cpp
ENDLINE = 0x0B  # next row
PAUSE = 0x0A    # wait, then erase the screen backwards (new "page")
STOP = 0x0C     # end of the script, added automatically

GLYPH_SLOTS = 8  # bytes 0x00-0x07 show the custom glyphs loaded by defineFont(0)

# typeEffect() walks the array with a uint8_t index, so past 255 bytes the
# index wraps around and the credits would loop forever.
MAX_BYTES = 255

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_INFO_CPP = REPO_ROOT / "src" / "state" / "info.cpp"

ARRAY_RE = re.compile(r"(static\s+const\s+char\s+opcodes\[\]\s+PROGMEM\s*=\s*)\{([^}]*)\};")

ESCAPE_HELP = r"""escapes:
  \0 .. \7   custom glyph in CGRAM slot 0-7 (one digit only, \12 is glyph 1 then '2')
  \n         next row
  \p         pause, then erase the screen (starts a new page)
  \xHH       any byte, exactly two hex digits
  \\         a backslash (note: most HD44780 ROMs draw 0x5C as a yen sign)"""


class CreditsError(Exception):
    pass


def encode(text):
    """Escaped text -> list of byte values, without the STOP terminator."""
    out = []
    i = 0
    while i < len(text):
        ch = text[i]
        if ch != "\\":
            code = ord(ch)
            if code > 0x7F:
                raise CreditsError(f"non-ASCII character {ch!r} at position {i}, use \\xHH if you really want that byte")
            out.append(code)
            i += 1
            continue

        if i + 1 >= len(text):
            raise CreditsError("the text ends with a lone backslash")

        esc = text[i + 1]
        if esc in "01234567":
            out.append(int(esc))
            i += 2
        elif esc == "n":
            out.append(ENDLINE)
            i += 2
        elif esc == "p":
            out.append(PAUSE)
            i += 2
        elif esc == "\\":
            out.append(ord("\\"))
            i += 2
        elif esc == "x":
            digits = text[i + 2:i + 4]
            if not re.fullmatch(r"[0-9a-fA-F]{2}", digits):
                raise CreditsError(f"\\x at position {i} needs exactly two hex digits")
            out.append(int(digits, 16))
            i += 4
        else:
            raise CreditsError(f"unknown escape \\{esc} at position {i}")

    if STOP in out:
        raise CreditsError("byte 0x0c is the terminator, it can't appear inside the text (it gets added for you)")
    if len(out) + 1 > MAX_BYTES:
        raise CreditsError(f"the script is {len(out) + 1} bytes with the terminator, typeEffect() can only walk {MAX_BYTES}")
    return out


def decode(data):
    """List of byte values -> escaped text. Stops at the first STOP byte."""
    parts = []
    for b in data:
        if b == STOP:
            break
        if b < GLYPH_SLOTS:
            parts.append(f"\\{b}")
        elif b == ENDLINE:
            parts.append("\\n")
        elif b == PAUSE:
            parts.append("\\p")
        elif b == ord("\\"):
            parts.append("\\\\")
        elif 0x20 <= b <= 0x7E:
            parts.append(chr(b))
        else:
            parts.append(f"\\x{b:02x}")
    return "".join(parts)


def to_cpp_line(data):
    body = ", ".join(f"0x{b:02x}" for b in list(data) + [STOP])
    return f"static const char opcodes[] PROGMEM = {{{body}}};"


def parse_array(source):
    """Pulls the byte list out of a C++ line or a whole info.cpp."""
    m = ARRAY_RE.search(source)
    body = m.group(2) if m else None
    if body is None:
        # Also accept a bare "{0x54, 0x68, ...}" or just the numbers
        m = re.search(r"\{([^}]*)\}", source)
        body = m.group(1) if m else source

    values = []
    for tok in re.findall(r"0[xX][0-9a-fA-F]+|\d+", body):
        v = int(tok, 0) if tok.lower().startswith("0x") else int(tok)
        if v > 0xFF:
            raise CreditsError(f"{tok} doesn't fit in a byte")
        values.append(v)
    if not values:
        raise CreditsError("couldn't find any bytes in the input")
    return values


def apply_to_file(path, data):
    path = Path(path)
    src = path.read_text(encoding="ascii")
    new_line = to_cpp_line(data)
    new_src, count = ARRAY_RE.subn(lambda _: new_line, src)
    if count != 1:
        raise CreditsError(f"expected exactly one opcodes array in {path}, found {count}")
    path.write_bytes(new_src.encode("ascii"))


def check_layout(data, cols, rows):
    """Warnings for text that won't fit the screen. Doesn't stop anything."""
    warnings = []
    for p, page in enumerate(split_pages(data), start=1):
        if len(page) > rows:
            warnings.append(f"page {p} has {len(page)} rows, the display has {rows} (extra rows wrap to the top)")
        for r, line in enumerate(page, start=1):
            if len(line) > cols:
                warnings.append(f"page {p}, row {r} is {len(line)} characters, only {cols} are visible")
    return warnings


def split_pages(data):
    """Bytes -> [[row bytes, ...], ...] following typeEffect()'s rules."""
    pages = [[[]]]
    for b in data:
        if b == STOP:
            break
        if b == PAUSE:
            pages.append([[]])
        elif b == ENDLINE:
            pages[-1].append([])
        else:
            pages[-1][-1].append(b)
    return pages


def preview(data, cols, rows):
    """Text drawing of every page, with glyphs as '*'."""
    lines = []
    border = "+" + "-" * cols + "+"
    for p, page in enumerate(split_pages(data), start=1):
        lines.append(f"page {p}")
        lines.append(border)
        for r in range(max(rows, len(page))):
            row = page[r] if r < len(page) else []
            cells = "".join("*" if b < GLYPH_SLOTS else (chr(b) if 0x20 <= b <= 0x7E else "?") for b in row)
            marker = "" if r < rows else "   <- off screen"
            lines.append("|" + cells[:cols].ljust(cols) + "|" + (cells[cols:] and f" {cells[cols:]!r} cut off") + marker)
        lines.append(border)
    lines.append("* = custom glyph (CGRAM slot)")
    return "\n".join(lines)


def die(msg):
    print(f"error: {msg}", file=sys.stderr)
    sys.exit(1)
