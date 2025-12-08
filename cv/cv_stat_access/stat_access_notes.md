# 🧭 OSY – Súbory: `stat()`, `lstat()`, `fstat()`, `access()`

## 1️⃣ `stat()` – načítanie METADÁT súboru

### 💡 Účel
Získať informácie o súbore (veľkosť, typ, oprávnenia, časy úprav, vlastníka…).

### 🔧 Deklarácia
```c
int stat(const char *path, struct stat *buf);
```

- **`path`** – cesta k súboru (napr. `"file.txt"`, `"/etc/passwd"`).  
- **`buf`** – ukazovateľ na štruktúru, kam sa údaje uložia.  
- **Návratová hodnota:**  
  `0` → úspech  
  `-1` → chyba (napr. neexistuje súbor, chýba oprávnenie)

### 🧩 Štruktúra `struct stat`
```c
struct stat {
    mode_t  st_mode;   // typ + prístupové práva
    off_t   st_size;   // veľkosť v bajtoch
    time_t  st_mtime;  // čas poslednej modifikácie
    uid_t   st_uid;    // vlastník (UID)
    gid_t   st_gid;    // skupina (GID)
    nlink_t st_nlink;  // počet hardlinkov
};
```

### 📘 Príklad použitia
```c
struct stat info;
if (stat("file.txt", &info) == 0) {
    printf("Veľkosť: %lld B\n", (long long)info.st_size);
    printf("Posledná modifikácia: %s", ctime(&info.st_mtime));
}
```

### 🧠 Zistenie typu súboru (z `st_mode`)
```c
S_ISREG(st_mode)  // bežný súbor
S_ISDIR(st_mode)  // adresár
S_ISLNK(st_mode)  // symbolický link
S_ISCHR(st_mode)  // znakové zariadenie
S_ISBLK(st_mode)  // blokové zariadenie
S_ISFIFO(st_mode) // rúra
S_ISSOCK(st_mode) // socket
```

### ⚙️ Varianty `stat`
| Funkcia | Popis |
|----------|--------|
| `stat()` | info o súbore (ak je link, sleduje cieľ) |
| `lstat()` | ak je symlink, vráti info o linku |
| `fstat()` | info o už otvorenom deskriptore (file descriptor) |

## 2️⃣ `access()` – test prístupových práv / existencie

### 💡 Účel
Overiť, či **aktuálny proces** má právo so súborom niečo urobiť (čítať, zapisovať, spustiť, či vôbec existuje).

### 🔧 Deklarácia
```c
int access(const char *path, int mode);
```

- **`path`** – cesta k súboru  
- **`mode`** – testovaný prístup (možno kombinovať pomocou `|`)

| Konštanta | Význam |
|------------|---------|
| `F_OK` | súbor **existuje** |
| `R_OK` | máš právo **čítať** |
| `W_OK` | máš právo **zapisovať** |
| `X_OK` | máš právo **spúšťať** |

**Návratová hodnota:**
- `0` → prístup povolený  
- `-1` → prístup odmietnutý (alebo súbor neexistuje)

### 📘 Príklad použitia
```c
if (access("program.sh", X_OK) == 0)
    printf("Môže sa spustiť!\n");
else
    printf("Nie je spustiteľný.\n");
```

## 3️⃣ Kedy použiť čo

| Funkcia | Kedy ju použiť |
|----------|----------------|
| `stat()` / `lstat()` / `fstat()` | keď potrebuješ podrobné informácie (veľkosť, typ, časy, práva) |
| `access()` | keď sa rozhoduješ, či máš oprávnenie niečo so súborom spraviť |

### Typický workflow
```c
struct stat st;
if (stat(path, &st) == 0) {
    if (S_ISREG(st.st_mode) && access(path, R_OK) == 0) {
        // súbor existuje, je bežný a čitateľný
        // → môžem ho otvoriť
    }
}
```

## 4️⃣ Chyby a tipy

- Po `-1` vždy kontroluj `errno` (napr. cez `perror("stat")` alebo `perror("access")`).
- `ctime(&st.st_mtime)` vracia reťazec s časom modifikácie (obsahuje `\n`).
- Režim (práva) vypíšeš v oktále:
  ```c
  printf("Režim: %o\n", st.st_mode);
  ```
- Oprávnenia v bitech:
  ```c
  S_IRUSR  // vlastník – čítanie
  S_IWUSR  // vlastník – zápis
  S_IXUSR  // vlastník – spustenie
  S_IRGRP, S_IROTH ...
  ```

## 5️⃣ Rýchle Q&A

| Otázka | Odpoveď |
|---------|----------|
| `stat("neexistuje.txt", &st)` | vráti `-1`, `errno = ENOENT` |
| `lstat("link.txt", &st)` na symlink | vráti info o **linku**, nie o cieľovom súbore |
| `access("tajny.txt", R_OK)` bez oprávnenia | vráti `-1`, `errno = EACCES` |

## 💡 Zhrnutie
| Funkcia | Účel | Návrat |
|----------|------|---------|
| `stat()` | získa info o súbore | `0` / `-1` |
| `lstat()` | info o linku | `0` / `-1` |
| `fstat()` | info o otvorenom súbore (fd) | `0` / `-1` |
| `access()` | testuje oprávnenie / existenciu | `0` / `-1` |
