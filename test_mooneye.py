import subprocess
from pathlib import Path

ROMDIR = Path("/home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/emulator-only/mbc1")
EMU = Path("/home/max/Documents/stuff/programming/repos/gb-emu/gbemu")

def main():
    testfiles = list(ROMDIR.glob("./**/*.gb"))
    print(f"Found {len(testfiles)} test files")
    failed = []
    for testfile in testfiles:
        proc = subprocess.Popen([EMU, testfile], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        proc.wait()
        out, err = proc.communicate()
        if "Breakpoint B/C/D/E/H/L = 03/05/08/0d/15/22" not in err.decode('utf-8'):
            failed.append(testfile)
    print(f"Failed {len(failed)} tests")
    if len(failed) > 0:
        print("Failed tests:")
        for f in failed:
            print(f"  {f}")

if __name__ == "__main__":
    main()