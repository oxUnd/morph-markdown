#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
PATTERNS = ["*.swift", "*.c", "*.h", "*.m", "*.mm", "*.cc", "*.cpp"]
LIMIT = 200


def files():
    for pattern in PATTERNS:
        yield from ROOT.rglob(pattern)


def swift_functions(lines):
    starts = re.compile(r"^\s*(public |internal |private |fileprivate |open )?(final )?(class |struct |enum |protocol |extension |func |static func |override func )")
    for index, line in enumerate(lines):
        if "func " in line or starts.match(line):
            yield index


def c_functions(lines):
    starts = re.compile(r"^[A-Za-z_][A-Za-z0-9_ *\t]*\([^;]*\)\s*\{")
    for index, line in enumerate(lines):
        if starts.match(line.strip()):
            yield index


def span(lines, start):
    depth = 0
    seen = False
    for index in range(start, len(lines)):
        depth += lines[index].count("{") - lines[index].count("}")
        seen = seen or "{" in lines[index]
        if seen and depth <= 0:
            return index - start + 1
    return 1


def main():
    violations = []
    for path in files():
        if ".build" in path.parts:
            continue
        lines = path.read_text(errors="ignore").splitlines()
        starts = swift_functions(lines) if path.suffix == ".swift" else c_functions(lines)
        for start in starts:
            count = span(lines, start)
            if count > LIMIT:
                violations.append((path, start + 1, count))
    for path, line, count in violations:
        print(f"{path}:{line} function is {count} lines")
    return 1 if violations else 0


if __name__ == "__main__":
    sys.exit(main())
