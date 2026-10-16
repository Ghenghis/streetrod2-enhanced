#!/usr/bin/env python3
"""Headless geometry acceptance of the enhanced view (ENHANCED.md §3.10 item 3, §5): SR_ENH_COMPARE=2.

    python tools/enh_compare.py [--exe EXE] [--min PCT] [--dump]

Runs the demo drag race (road mode 1) and the drive home (town, mode 0) with the compare clock and the compare tool at
the default resolution (1280x800, 2x2) and --motion-delay 0. For every race frame the tool renders the frame's snapshot
(player position quantized as the original's row depths), takes the centre sample of each logical pixel, lays the
original's passthrough pixels (sprites, mirror) over it and matches the colour edges against the original's page:
an edge is matched when the enhanced picture has the same edge within +-1 px (or, next to a textured background, the
boundaries of both colours within +-1 px). Both directions (original edges found in the enhanced picture and the
other way round), horizontal and vertical edges, rows from the top of the ground to the split.
Prints the matched share per race and mode; exit code 0 when every share is at least --min (default 96 %).
--dump writes cmpNNNNN.bmp (original / enhanced / difference) every 25 frames to work/ec/cmp.
"""
import argparse, os, re, shutil, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ENHANCED = os.path.join(ROOT, "build", "sr2-enhanced.exe")
MODES = {0: "town", 1: "drag"}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", default=ENHANCED)
    ap.add_argument("--min", type=float, default=96.0)
    ap.add_argument("--dump", action="store_true")
    a = ap.parse_args()
    os.environ["PATH"] = r"C:\msys64\mingw64\bin" + os.pathsep + os.environ.get("PATH", "")
    base = os.path.join(ROOT, "work", "ec")
    shutil.rmtree(base, ignore_errors=True)
    game, log, cmp = os.path.join(base, "game"), os.path.join(base, "compare.log"), os.path.join(base, "cmp")
    os.makedirs(cmp)
    shutil.copytree(os.path.join(ROOT, "Game"), game, ignore=shutil.ignore_patterns("*.exo"))
    env = dict(os.environ, SDL_VIDEO_DRIVER="dummy", SDL_AUDIO_DRIVER="dummy", SR_SEED="1", SR_KEYS="1:03",
               SR_DEBUG_RACE="drag", SR_ENH_COMPARE="2", SR_ENH_COMPARE_LOG=log)
    if a.dump:
        env["SR_ENH_COMPARE_DIR"] = cmp
    subprocess.run([a.exe, "--game-dir", game, "--run-ms", "60000", "--motion-delay", "0", "demo", "v"], env=env,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=600)
    mode, ok, races = None, True, 0
    for line in open(log):
        m = re.match(r"K \d+ (\d+) ", line)
        if m:
            mode = int(m.group(1))
        m = re.match(r"K race frames (\d+)\s+h (\d+)/(\d+) (\d+)/(\d+)\s+v (\d+)/(\d+) (\d+)/(\d+)\s+worst frame (\d+) ([\d.]+)%",
                     line)
        if not m:
            continue
        races += 1
        n = [int(x) for x in m.groups()[:9]]
        shares = [100.0 * n[i] / max(n[i + 1], 1) for i in (1, 3, 5, 7)]
        good = min(shares) >= a.min
        ok &= good
        print("%-5s %-4s %4d frames  h %.2f%% / %.2f%%  v %.2f%% / %.2f%%  (original in enhanced / enhanced in original)"
              "  worst frame %s %.2f%%" % (MODES.get(mode, mode), "ok" if good else "FAIL", n[0], *shares,
                                           m.group(10), float(m.group(11))))
    if not races:
        print("FAIL: no compared race in", log)
    return 0 if ok and races else 1


if __name__ == "__main__":
    sys.exit(main())
