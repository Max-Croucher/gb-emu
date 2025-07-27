import subprocess
from pathlib import Path


IGNORE_FILES = {
    Path("/home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/boot_hwio-S.gb"),
    Path("/home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/boot_div-S.gb"),
    Path("/home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/boot_div2-S.gb"),
    Path("/home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/boot_regs-sgb.gb"),
    Path("/home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/boot_regs-mgb.gb"),
    Path("/home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/boot_regs-sgb2.gb"),
    Path("/home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/boot_hwio-dmg0.gb"),
    Path("/home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/boot_div-dmg0.gb"),
    Path("/home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/boot_regs-dmg0.gb")

}



ROMDIRS = [
    # Path("/home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/emulator-only/mbc1"),
    # Path("/home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/emulator-only/mbc2"),
    # Path("/home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/emulator-only/mbc5")
    Path("/home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance"),
]
EMU = Path("/home/max/Documents/stuff/programming/repos/gb-emu/gbemu")

TIMEOUT = 3

def main():
    testfiles = set(sum((list(romdir.glob("./**/*.gb")) for romdir in ROMDIRS), start=[]))
    testfiles -= IGNORE_FILES
    print(f"Found {len(testfiles)} test files")
    failed = []
    unknown = []
    timeouts = []
    for testfile in testfiles:
        print(f"{testfile}... ", end='')
        try:
            proc = subprocess.Popen([EMU, testfile, '--halt-on-breakpoint', '--no-save', '--screenshot-on-halt', '--max-speed', '--no-audio'], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
            proc.wait(TIMEOUT)
        except subprocess.TimeoutExpired:
            print("Timeout")
            proc.kill()
            timeouts.append(testfile)
        else:
            _, err = proc.communicate()
            if "BREAKPOINT FAILURE B/C/D/E/H/L = 42/42/42/42/42/42" in err.decode('utf-8'):
                print("Failure")
                failed.append(testfile)
            elif "BREAKPOINT SUCCESS B/C/D/E/H/L = 03/05/08/0d/15/22" not in err.decode('utf-8'):
                print("Unknown")
                unknown.append(testfile)
            else:
                print("Passed!")
    print(f"Passed {len(testfiles) - len(failed) - len(unknown) - len(timeouts)} tests")
    if len(failed) > 0:
        print("Failed tests:")
        for f in failed:
            print(f"  {f}")
    if len(unknown) > 0:
        print(f"No codes found in {len(unknown)} tests. Unable to verify success or failure.")
        print("Unknown tests:")
        for f in unknown:
            print(f"  {f}")
    if len(timeouts) > 0:
        print(f"{len(timeouts)} tests did not terminate after {TIMEOUT} seconds.")
        print("Non-Terminating tests:")
        for f in timeouts:
            print(f"  {f}")

if __name__ == "__main__":
    main()

# Failed tests:
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/ppu/lcdon_write_timing-GS.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/ppu/lcdon_timing-GS.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/serial/boot_sclk_align-dmgABCmgb.gb
# 30 tests did not terminate after 3 seconds.
# Non-Terminating tests:
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/ppu/intr_2_0_timing.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/call_cc_timing.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/oam_dma_restart.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/halt_ime1_timing2-GS.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/di_timing-GS.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/jp_timing.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/ppu/vblank_stat_intr-GS.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/ret_cc_timing.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/ppu/intr_2_mode0_timing_sprites.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/jp_cc_timing.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/ppu/intr_2_mode0_timing.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/ppu/intr_2_oam_ok_timing.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/add_sp_e_timing.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/oam_dma_timing.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/reti_timing.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/push_timing.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/oam_dma_start.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/ppu/intr_2_mode3_timing.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/ppu/hblank_ly_scx_timing-GS.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/ppu/stat_irq_blocking.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/halt_ime0_nointr_timing.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/ppu/intr_1_2_timing-GS.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/ppu/stat_lyc_onoff.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/ret_timing.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/call_timing.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/call_cc_timing2.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/ld_hl_sp_e_timing.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/call_timing2.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/rst_timing.gb
#   /home/max/Documents/stuff/programming/repos/gb-emu/test_roms/mts/acceptance/halt_ime0_ei.gb