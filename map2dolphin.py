import sys
import re

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python map2dolphin.py <input_file> <offset>")
        sys.exit(1)

    offset = int(sys.argv[2], 16)

    symInfo: list[list[str]] = []
    with open(sys.argv[1], "r") as f:
        lines = f.readlines()
        start = 0
        for i, line in enumerate(lines):
            if re.search(r"^.text", line.strip()):
                start = i + 4
                break

        for i, line in enumerate(lines[start:]):
            if line.strip() == "":
                break

            parts = line.split()
            symInfo.append(parts)

    with open(sys.argv[1][:-4] + ".dolphin.map", "w") as f:
        f.write(".text\n")
        for parts in symInfo:
            if parts[4] != "4":
                continue

            newaddr = int(parts[0], 16) + offset

            symbol = parts[5]
            if symbol.startswith("__") and symbol.count("__") == 1:
                pass
            else:
                symbol = symbol.rsplit("__", 1)[0]

            f.write(f"{newaddr:x} {parts[1]} {newaddr:x} 0 {symbol}\n")
