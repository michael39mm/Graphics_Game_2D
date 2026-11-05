# build_atlas.py
# el sheet el gdeed: grid 8 x 6 mazbout, background magenta.
# el script da bs bysheel el magenta (y3mel alpha) w by7seb makan el rgleen.
#
# use:  python build_atlas.py atreides_sprite_sheet.png atreides_atlas.png
import sys
import numpy as np
from PIL import Image

SRC = sys.argv[1] if len(sys.argv) > 1 else "atreides_sprite_sheet.png"
DST = sys.argv[2] if len(sys.argv) > 2 else "atreides_atlas.png"
COLS, ROWS = 8, 6

img = np.array(Image.open(SRC).convert("RGB")).astype(np.float32)
H, W, _ = img.shape
cw, ch = W // COLS, H // ROWS
img = img[:ch * ROWS, :cw * COLS]

R, G, B = img[..., 0], img[..., 1], img[..., 2]
# "magentaness": el background aali fe R w B w G wati
m = np.minimum(R, B) - G
alpha = np.clip(1.0 - (m - 40.0) / 120.0, 0.0, 1.0)
# despill: nshel el lon el magenta men el 7awaf
spill = np.maximum(m, 0.0) * 0.9
R2 = np.clip(R - spill, 0, 255)
B2 = np.clip(B - spill, 0, 255)
rgba = np.dstack([R2, G, B2, alpha * 255]).astype(np.uint8)
rgba[alpha < 0.02] = 0

# el rgleen: median bta3 a5er pixel solid fe row el idle
feet, cx = [], []
for c in range(COLS):
    a = rgba[0:ch, c * cw:(c + 1) * cw, 3]
    ys, xs = np.where(a > 230)
    feet.append(ys.max())
    cx.append((xs.min() + xs.max()) / 2.0)
print("cell", cw, "x", ch)
print("ANCHOR_X =", int(np.median(cx)), " ANCHOR_Y =", int(np.median(feet)) + 1)

Image.fromarray(rgba, "RGBA").save(DST)
print("saved", DST, rgba.shape[1], "x", rgba.shape[0])