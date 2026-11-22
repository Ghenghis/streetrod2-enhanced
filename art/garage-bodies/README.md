# Garage body art

In Enhanced mode pass `--body-art DIR` to enable parked stock-body replacements.
The filename is the three-digit LIB2 member number. Use a 32-bit BMP with
explicit alpha at four times the native member dimensions. An asset is eligible
only when every 4 × 4 alpha block agrees with its live native mask.

The default shows the period finish. Add `--body-art-restored` to use a
matching `NNN-restored.bmp` with stronger paint highlights and reflections.
The period finish loads when the restored asset is missing or fails the same
native mask check.

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

The 1965 Mustang also has a restored finish. Gemini used two Drive scene
references and garage masters for the bright roof and shoulder highlights, with a sharp horizon reflection and deeper lower panels. Both
finishes use the same 960 × 228 canvas, native mask and game-selected paint.
The garage replay confirmed zero changed pixels outside that mask.

## 033 — 1966 Ford Galaxie

This original Gemini body uses the sloping two-door hardtop design from the
period reference and the approved side-view illustration. Its 1024 × 220 image
fits SR2 model 02 at logical `(32, 104)` with the original `256 × 54` drawn
rectangle, axle placement, foreground, and live paint. The garage replay
changed zero pixels outside the native body mask.
[Ford’s 1966 full-size sales brochure](https://over-drive-magazine.com/wp-content/uploads/2024/11/1966-Ford-Full-Size-Brochure-1-24C.pdf)

## 090 — 1961 Ford Thunderbird

This original Gemini illustration depicts the fixed-roof hardtop with the
shallow rear wheel opening visible in the period reference. Its 960 × 220
image uses SR2 model 09's native mask and wheel openings. The game drew the
body at logical `(40, 104)` with its original `240 × 54` drawn rectangle.
The stock-versus-Enhanced garage replay found zero changed pixels outside
the body mask.
[Ford's 1961 Thunderbird archive photograph](https://fordimages.com/products/0400-8375-jpg)

## 040 — 1968 Ford Torino

This original Gemini illustration follows the SportsRoof profile in Ford's
period Torino advertisement. The fitted 992 × 216 asset uses SR2 model 05's
native mask. The garage replay drew its original 248 × 53 rectangle at
logical (40, 105) and changed zero pixels outside the body mask.
[Ford’s 1968 Torino advertisement](https://autohistorypreservationsociety.org/wp-content/uploads/1968-FORD-Ad-Torino-1.pdf)

## 026 — 1963 Ford Falcon

The 1024 × 220 Gemini body follows Ford's two-door hardtop and retains the
slender side trim and front fender spear. Model 01 uses the exact native
mask at logical (32, 104). The garage replay changed zero pixels outside
its body mask.
[1963 Ford archive photograph](https://fordimages.com/products/hv1963-0039)

## 039 — 1964 Ford Fairlane

The 992 × 232 Gemini body retains the Sports Coupe's side chrome and slanted
trim accent. The fitting preserves the straight molding and the game tint.
Model 04 uses the exact native mask at logical (40, 101). Its garage replay
changed zero pixels outside the body mask.
[1964 Ford archive photograph](https://fordimages.com/products/hv1964-0020)

## 089 — 1967 Mercury Cougar

The 1024 × 216 Gemini body uses the early Cougar's long hood and short rear
deck. Model 08 draws it at logical (32, 105) with its exact native mask.
Its garage replay changed zero pixels outside the body mask.
[1967 Mercury factory brochure archive](https://cougarclub2.org/collectibles/brochures/)
