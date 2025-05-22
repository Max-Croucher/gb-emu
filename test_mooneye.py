import subprocess
from pathlib import Path

ROMDIR = Path("/home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/emulator-only/mbc1")
EMU = Path("/home/max/Documents/stuff/programming/repos/gb-emu/gbemu")

def main():
    testfiles = list(ROMDIR.glob("./**/*.gb"))
    print(f"Found {len(testfiles)} test files")
    for testfile in testfiles:
        print(testfile)
        proc = subprocess.Popen([EMU, testfile], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        proc.wait()
        err = proc.communicate()
        print(err)
        input()

if __name__ == "__main__":
    main()