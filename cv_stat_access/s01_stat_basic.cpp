#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

/*
===========================================================
 Úloha S1 – stat(): Základné info o jednom súbore (OSY)
===========================================================

🎯 Cieľ:
- Načítať a vypísať METADÁ údaje o súbore pomocou stat():
  - názov, veľkosť v bajtoch, typ (súbor/adresár/iné),
  - čas poslednej modifikácie,
  - práva (mode) v oktálovej forme.

📥 Vstup:
- Program prijme presne 1 argument: cestu k súboru.
  Príklad:  ./s01_stat_basic /etc/passwd

🔧 Postup (kroky):
1) Skontroluj počet argumentov. Pri nesprávnom počte vypíš chybu na stderr a skonči (return 1).
2) Vytvor premennú:  struct stat info;
3) Zavolaj:  if (stat(argv[1], &info) == -1) { perror("stat"); return 1; }
4) Z info vypíš:
   - "Subor: <argv[1]>"
   - "Velkost: <info.st_size> B"   (cast na long long pri printf)
   - "Typ: subor/adresar/iny"      (použi S_ISREG / S_ISDIR / inak "iny")
   - "Cas poslednej modifikacie: <ctime(&info.st_mtime)>" (vracia reťazec s \n)
   - "Prava (mode): %o"            (printf s %o; pozri info.st_mode)
5) Každý printf ukonči \n (buffering).

📚 Potrebné hlavičky:
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

💡 Užitočné makrá:
- S_ISREG(info.st_mode)  → bežný súbor
- S_ISDIR(info.st_mode)  → adresár
- S_ISLNK(info.st_mode)  → symbolický link (ak chceš rozšíriť)
- (lstat() ti dá info o linku samotnom)

🧪 Očakávaný výstup (príklad):
Subor: s01_stat_basic.cpp
Velkost: 483 B
Typ: subor
Cas poslednej modifikacie: Sat Oct 26 11:45:32 2025
Prava (mode): 100644

🛠️ Build:
g++ -std=c++17 -Wall -Wextra -O2 s01_stat_basic.cpp -o s01_stat_basic

▶️ Spustenie:
./s01_stat_basic /etc/passwd
./s01_stat_basic s01_stat_basic.cpp

⚠️ Pozor:
- Pri neexistujúcom súbore stat() vráti -1 → skontroluj a vypíš perror("stat").
- Pri formátovaní času ctime() už obsahuje koncové \n.
- Pri veľkosti použij:  printf("%lld", (long long)info.st_size);

✅ Bonus (dobrovoľné rozšírenie):
- Vypíš aj vlastníka (st_uid), skupinu (st_gid), počet hardlinkov (st_nlink).
- Vypíš typy aj pre FIFO/CHR/BLK/SOCK.
*/

int main(int argc, char *argv[]) {

    if (argc != 2)
    {
        fprintf(stderr, "Neplatny pocet argumentov\n");
        return 1;
    }

    struct stat info;
    if(stat(argv[1], &info) == -1) {
        perror("stat zlyhal"); 
        return 1;
    }
    else
    {
        printf("Nazov: %s, ", argv[1]);
        printf("Velkost: %ld, ", (long)info.st_size);
        printf("Typ: ");
        if (S_ISDIR(info.st_mode))
        {
            printf("adresar, ");
        }
        else if (S_ISREG(info.st_mode))
        {
            printf("subor, ");
        }
        else
        {
            printf("iny, ");
        }
        printf("\nCas poslednej modifikacie, %s", ctime(&info.st_mtime));
        printf("Prava: %o\n", info.st_mode);
        printf("Vlastnik: %u\n", info.st_uid);
        printf("Skupina: %u\n", info.st_gid);
    }
    
    return 0;
}