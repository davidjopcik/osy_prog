// cheatsheet_xz_imagemagick.cpp
// Mini-cheatsheet pre xz + ImageMagick (magick/identify/display)
// Spusti a vypíše stručný prehľad príkazov do stdout.

#include <iostream>

int main() {
    const char* CHEATSHEET = R"(

========================
XZ – kompresia/dekompresia
========================
# Zbalenie do .xz (zmaže pôvodný súbor)
xz file

# Zbalenie, ale ponechaj pôvodný
xz -k file

# Rozbalenie .xz
xz -d file.xz
# alias
unxz file.xz

# Rozbalenie bez zmazania archívu
xz -dk file.xz

# Streaming (stdin/stdout)
xz -c file > out.xz        # kompresia na stdout
xz -dc file.xz > out       # dekompresia na stdout

# Test archívu bez rozbalenia
xz -t file.xz

# Úroveň kompresie (0–9, default ~6)
xz -9 file      # maximálna kompresia


=====================================
ImageMagick – magick / identify / display
=====================================
# Konverzia formátu
magick in.jpg out.png

# Zmena rozmeru (presne šírka×výška; '!' = ignoruj pomer strán)
magick in.png -resize 1500x750! out.png

# Čítanie zo stdin / zápis na stdout
magick - out.png              # '-' = stdin
magick in.png png:- > out.png # 'png:-' = stdout

# Rýchle zistenie rozmerov
identify -format "%wx%h\n" obrazok.png

# Zobrazenie (ak je dostupné X11/GUI)
display obrazok.png


=====================
Užitočné pipeline-y
=====================
# Server: vygeneruj PNG a hneď komprimuj do .xz
magick podzim.png -resize 1500x750! png:- | xz -c > image.img

# Klient: prijatý image.img rozbaľ do PNG súboru
xz -dc image.img > out.png

# Klient: rozbaľ a rovno spracuj v ImageMagick
xz -dc image.img | magick - out.png


Poznámky:
- V xz:
  - -c  → píš na stdout
  - -d  → dekomprimuj
  - -t  → test archívu
- V ImageMagick:
  - '-' znamená stdin
  - 'format:-' znamená stdout v danom formáte (napr. png:-)
  - 'identify' je súčasť IM, vhodné na rýchlu kontrolu.
)";

    std::cout << CHEATSHEET << std::endl;
    return 0;
}