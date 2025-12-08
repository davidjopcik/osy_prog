/*
===========================================================
 OSY – Súbory: stat(), lstat(), fstat(), access() – rýchly prehľad
===========================================================

1) stat() – načítanie METADÁT súboru
------------------------------------
- Účel: získať informácie o súbore (veľkosť, typ, práva, časy).
- Deklarácia: int stat(const char *path, struct stat *buf);
- Návrat: 0 = OK, -1 = chyba (errno nastavené, napr. ENOENT).
- Kľúčové polia v struct stat:
    mode_t  st_mode;   // typ + práva
    off_t   st_size;   // veľkosť v bajtoch
    time_t  st_mtime;  // čas poslednej modifikácie
    uid_t   st_uid;    // vlastník (UID)
    gid_t   st_gid;    // skupina (GID)
    nlink_t st_nlink;  // počet hardlinkov

- Zistenie typu súboru (z st_mode):
    S_ISREG(st_mode)  // bežný súbor
    S_ISDIR(st_mode)  // adresár
    S_ISLNK(st_mode)  // symbolický link
    S_ISCHR(st_mode)  // zn. zariadenie
    S_ISBLK(st_mode)  // blok. zariadenie
    S_ISFIFO(st_mode) // rúra (FIFO)
    S_ISSOCK(st_mode) // socket

- Varianty:
    lstat(path, &st)  // neprechádza symlink – vráti info o LINKU
    fstat(fd, &st)    // info o už otvorenom deskriptore fd

- Mini príklad použitia (len ilustrácia, nie vykonateľný kód):
    struct stat st;
    if (stat("file.txt", &st) == 0) {
        printf("Velkost: %lld B\n", (long long)st.st_size);
        printf("Modifikacia: %s", ctime(&st.st_mtime));
        if (S_ISDIR(st.st_mode)) puts("Je to adresar.");
    } else {
        perror("stat");
    }


2) access() – test prístupových PRÁV/EXISTENCIE
-----------------------------------------------
- Účel: overiť, či aktuálny proces *môže* so súborom niečo urobiť.
- Deklarácia: int access(const char *path, int mode);
- Návrat: 0 = povolené, -1 = chyba (napr. neexistuje alebo nedostatok práv).
- Módy (môžeš kombinovať pomocou |):
    F_OK  // existuje?
    R_OK  // čitateľný?
    W_OK  // zapisovateľný?
    X_OK  // spustiteľný?

- Mini príklad:
    if (access("skript.sh", X_OK) == 0) puts("Spustitelny.");
    else                                puts("Nespustitelny alebo neexistuje.");


3) Kedy použiť čo
-----------------
- stat()/lstat()/fstat(): keď potrebuješ PODROBNÉ INFO (veľkosť, typ, časy, práva…)
- access(): keď sa rozhoduješ, či má zmysel skúsiť operáciu (čítať/zapisovať/spustiť)

- Typický workflow:
    if (stat(path, &st) == 0) {
        if (S_ISREG(st.st_mode) && access(path, R_OK) == 0) {
            // je to regulárny súbor a je čitateľný → otvor, čítaj…
        }
    }


4) Chyby a tipy
---------------
- Po -1 vždy kontroluj errno (perror("stat") / perror("access")).
- Pri výpisoch konči riadok '\n' (buffering).
- Na formátovanie času: ctime(&st.st_mtime) (vracia reťazec s '\n').
- Práva v oktále (ako ls -l) vieš zobraziť cez printf("%o", st.st_mode).

-----------------------------------------------
 Rýchle Q&A:
  Q: stat("neexistuje")? → -1, errno=ENOENT
  Q: lstat("link") na symlink? → info o linku, nie o cieli
  Q: access(path, R_OK)? → 0 ak môžem čítať ako aktuálny proces
-----------------------------------------------
*/