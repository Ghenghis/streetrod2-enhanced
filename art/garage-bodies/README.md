# Garage body art

In Enhanced mode pass `--body-art DIR` to enable parked stock-body replacements.
The filename is the three-digit LIB2 member number. Use a 32-bit BMP with
explicit alpha at four times the native member dimensions. An asset is eligible
only when every 4 × 4 alpha block agrees with its live native mask.

## 034 — 1965 Ford Mustang Fastback 2+2

This is an original Gemini illustration fitted to SR2 model 03, LIB2 member 034.
The original 960 × 228 asset contains the body and preserves the native wheel
and window openings. The runtime uses the game’s selected paint hue with the
illustration’s own shading. The renderer checks native foreground ownership before
applying the body, so stock tyres and foreground elements keep their placement.
[Ford’s 1965 sales brochure](https://www.auto-brochures.com/makes/Ford/Mustang/Ford_US%20Mustang_1965.pdf)

The stock garage parks this body at logical `(40, 102)` on the 320 × 200
screen and draws 56 of its 57 rows. Cars without an eligible replacement,
modified bodies, Classic mode and other scenes use the original sprites.
