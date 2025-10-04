# OSY – Unix/Linux Helper (krátka verzia)

## Navigácia a súbory
```
pwd                # Zobraz aktuálny priečinok
ls -la             # Výpis súborov (aj skryté)
cd /path           # Zmena priečinka
mkdir -p dir/sub   # Vytvorenie priečinkov
cp src dst         # Kopírovanie
mv src dst         # Premenovanie/presun
rm -rf dir         # Zmazanie (pozor!)
touch file         # Nový prázdny súbor
cat file           # Zobrazenie obsahu
less file          # Prehliadanie s posunom
grep "text" *.c    # Vyhľadaj reťazec
find . -name "*.c" # Nájdite súbory
wc -l file         # Počet riadkov
du -sh .           # Veľkosť priečinka
df -h              # Voľné miesto na diskoch
```

## Práva a vlastníctvo
```
ls -l              # Zobraz práva a vlastníka
chmod 754 file     # Nastavenie práv (rwxr-xr--)
chown user:group f # Zmena vlastníka/skupiny
umask 022          # Predvolené odoberanie práv
```

## Presmerovanie a rúry
```
cmd > out.txt      # Výstup do súboru
cmd >> out.txt     # Pridanie na koniec
cmd 2> err.txt     # Chybový výstup
cmd1 | cmd2        # Prepojenie cez rúru
cmd < in.txt       # Vstup zo súboru
```

## Procesy a joby
```
ps aux             # Zoznam procesov
top                # Interaktívny výpis procesov
& / Ctrl+Z / bg / fg  # Pozadie, stop, pokračovanie
kill -9 PID        # Ukončenie procesu
jobs               # Zoznam úloh v shelli
time cmd           # Meranie času vykonania
```

## Kompilácia C/C++ a knižnice
```
gcc -Wall main.c -o app          # Preklad
ar rcs libmylib.a foo.o bar.o    # Statická knižnica
gcc -shared -fPIC -o libx.so x.o # Dynamická knižnica
gcc main.c -L. -lx -o app        # Linkovanie
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH  # Načítanie .so
ldd ./app                        # Zoznam .so knižníc
make / make clean                # Build cez Makefile
```

## Sieť a pripojenie
```
ssh user@server        # Pripojenie k serveru
scp file user@host:~/  # Kopírovanie po SSH
ping host              # Overenie konektivity
ip a / ip r            # Sieťové rozhrania / routovanie
```

## Archívy a kompresia
```
tar -czf file.tgz dir  # Zbalenie priečinka
tar -xzf file.tgz      # Rozbalenie archívu
gzip file / gunzip file# Kompresia/dekompresia
```

## Užitočné klávesy
```
Ctrl+C  # Ukončenie procesu
Ctrl+Z  # Zastavenie procesu
Ctrl+D  # Koniec vstupu (EOF)
```
