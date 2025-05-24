import subprocess
from pathlib import Path

ROMDIRS = [
    # Path("/home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/emulator-only/mbc1"),
    # Path("/home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/emulator-only/mbc2"),
    # Path("/home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/emulator-only/mbc5"),
    #Path("/home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance")
    Path("/home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/timer")
]
EMU = Path("/home/max/Documents/stuff/programming/repos/gb-emu/gbemu")

def main():
    testfiles = sum((list(romdir.glob("./**/*.gb")) for romdir in ROMDIRS), start=[])
    print(f"Found {len(testfiles)} test files")
    failed = []
    unknown = []
    for testfile in testfiles:
        proc = subprocess.Popen([EMU, testfile], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        proc.wait()
        out, err = proc.communicate()
        if "Breakpoint B/C/D/E/H/L = 42/42/42/42/42/42" in err.decode('utf-8'):
            failed.append(testfile)
        elif "Breakpoint B/C/D/E/H/L = 03/05/08/0d/15/22" not in err.decode('utf-8'):
            unknown.append(testfile)
    print(f"Passed {len(testfiles) - len(failed) - len(unknown)} tests")
    if len(failed) > 0:
        print("Failed tests:")
        for f in failed:
            print(f"  {f}")
    if len(unknown) > 0:
        print(f"No codes found in {len(unknown)} tests. Unable to verify success or failure.")
        print("Unknown tests:")
        for f in unknown:
            print(f"  {f}")


def test_div():
    testfile = Path("/home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/boot_div-dmgABCmgb.gb")
    for i in range(512):
        proc = subprocess.Popen([EMU, testfile, str(43776+i-256)], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        proc.wait()
        out, err = proc.communicate()
        if "Breakpoint B/C/D/E/H/L = 03/05/08/0d/15/22" in err.decode('utf-8'):
            print(f"passed with {hex(43776+i-256)}")



if __name__ == "__main__":
    test_div()