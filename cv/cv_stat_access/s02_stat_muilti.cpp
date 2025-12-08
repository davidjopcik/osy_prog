#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <string>

/*
===========================================================
 Úloha S2 – stat(): Info o viacerých súboroch (OSY)
===========================================================

🎯 Cieľ:
- Spracovať viacero ciest (argumentov programu) a pre každý súbor:
  - zistiť jeho typ (subor, adresar, link, atď.),
  - zistiť jeho veľkosť (st_size),
  - ošetriť chybu ak neexistuje alebo je neprístupný.

📥 Vstup:
- Program prijme ľubovoľný počet argumentov (min. 1).
  Príklad: ./s02_stat_multi /etc /bin/bash neexistuje

📤 Výstup:
- Pre každý argument vypíš jeden riadok:
    <nazov> : <typ>, velkost <st_size> B
- Ak sa stat() nepodarí:
    <nazov> : [chyba pri stat()]

📚 Použi:
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

🔧 Postup:
1) Skontroluj, že argc > 1 (inak vypíš použitie programu a skonči).
2) Pre každý argument od argv[1] po argv[argc-1]:
   a) Zavolaj stat(argv[i], &info);
   b) Ak vráti -1 → vypíš "<nazov>: [chyba pri stat()]" a pokračuj.
   c) Ak OK → urč typ súboru pomocou makier:
        S_ISREG(info.st_mode)  → "subor"
        S_ISDIR(info.st_mode)  → "adresar"
        S_ISLNK(info.st_mode)  → "link"
        S_ISFIFO(info.st_mode) → "fifo"
        S_ISCHR(info.st_mode)  → "znakove zariadenie"
        S_ISBLK(info.st_mode)  → "blokove zariadenie"
        S_ISSOCK(info.st_mode) → "socket"
        inak → "iny"
   d) Vypíš formát:
        printf("%s : %s, velkost %lld B\n", argv[i], typ, (long long)info.st_size);

🧪 Príklad výstupu:
-----------------------------------------------
./s02_stat_multi /etc /bin/bash neexistuje
/etc        : adresar, velkost 4096 B
/bin/bash   : subor, velkost 1183448 B
neexistuje  : [chyba pri stat()]
-----------------------------------------------

🛠️ Build:
g++ -std=c++17 -Wall -Wextra -O2 s02_stat_multi.cpp -o s02_stat_multi

⚠️ Pozor:
- stat() môže zlyhať pre neexistujúci alebo neprístupný súbor.
- Pri výpise veľkosti používaj (long long) cast.
- Každý printf ukonči \n (ináč sa buffer nevypíše).
- Nepoužívaj lstat() (tu sleduj linky až k cieľu).

✅ Bonus (dobrovoľné rozšírenie):
- Implementuj prepínač `--literal`, ktorý použije lstat() namiesto stat().
- Zarovnaj výpisy podľa najdlhšieho názvu (printf s %-*s).
*/


void print_info(const char *path, struct stat info) {
    const char *type = "iny";
    if (S_ISREG(info.st_mode))
    {
        type = "subor";
        
    }
    else if (S_ISDIR(info.st_mode))
    {
        type = "adresar";
    }

    printf("%s : typ: %s, velkost: %lld\n", path, type, (long long)info.st_size);
}


int main(int argc, char *argv[]) {
    int N = 50;
    struct stat info;
    if (argc < 2)
    {
        fprintf(stderr, "Malo argumentov");
        return 1;
    }
    if (argc > N)
    {
        fprintf(stderr, "Vela argumentov");
        return 1;
    } 

    for (int i = 1; i < argc; i++)
    {
        if (stat(argv[i], &info) == -1)
        {
            perror(argv[i]);
            continue;
        }
        print_info(argv[i], info);  
    }
    
    return 0;
}