// osy_cheatsheet.c
// Základy v C: stringy, polia, súbory (stdio + POSIX), printf/sprintf/snprintf, sscanf, read/write.
// Kompilácia: gcc -std=c17 -Wall -Wextra -O2 osy_cheatsheet.c -o osy

#define _GNU_SOURCE
#include <stdio.h>      // printf, FILE*, fopen, fgets, fputs, fclose, sscanf, snprintf, sprintf, getline
#include <string.h>     // strlen, strcpy, strncpy, strcat, strcmp, strtok, memset, strerror, strchr, strstr, strnlen, memmove
#include <ctype.h>      // isspace, isdigit
#include <errno.h>      // errno
#include <stdlib.h>     // exit, malloc, free, calloc, realloc, strtol, strtoul
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>      // open
#include <unistd.h>     // read, write, close, lseek

static void die(const char* msg) {
    fprintf(stderr, "[ERR] %s: %s\n", msg, strerror(errno));
}

// ------------------------------
// 0) UŽITOČNÉ POZNÁMKY k formátom
// ------------------------------
//  - %zu: size_t
//  - %ld / %lld: long / long long
//  - %.nf: desatinné miesta pre float/double
//  - %*s / %*d: šírka z parametra
//  - vždy kontroluj návratové hodnoty (sscanf/snprintf vracajú počet zapísaných/parsnutých vecí)

// ------------------------------
// 1) STRINGY
// ------------------------------
static void demo_strings(void) {
    printf("\n=== STRINGY (C) ===\n");

    char s[128] = "Ahoj";
    printf("s='%s', dlzka=%zu\n", s, strlen(s));

    // spojenie
    strcat(s, " svet");
    printf("po strcat: '%s'\n", s);

    // kopia
    char copy[128];
    strcpy(copy, s);
    printf("kopia: '%s'\n", copy);

    // porovnanie
    printf("strcmp(s,copy) = %d (0 == zhodne)\n", strcmp(s, copy));

    // bezpečné skladanie do buffera (snprintf) – odporúčaný
    char joined[128];
    int nw = snprintf(joined, sizeof(joined), "%s %d %s", "cislo", 42, "OK");
    if (nw < 0 || (size_t)nw >= sizeof(joined)) {
        printf("snprintf overflow/err\n");
    }
    printf("snprintf -> '%s'\n", joined);

    // POZOR: sprintf NEKONTROLUJE dĺžku – môže pretekať (nepoužívaj v praxi).
    // (Ukážka len edukačná – reálne dávaj prednosť snprintf.)
    char bad[8];
    sprintf(bad, "cislo=%d", 12345); // môže sa pretiecť, tu je to len príklad
    printf("sprintf (nebezpecne) -> '%s' (NEPOUZIVAJ v realite)\n", bad);

    // rozdelenie (tokenizácia) + uloženie do poľa
    char csv[] = "jedna,dva,tri,st,ri";
    const char* delim = ",";
    char* token = strtok(csv, delim);

    char tokens[10][32]; // max 10 tokenov
    int n_tokens = 0;
    while (token && n_tokens < 10) {
        strncpy(tokens[n_tokens], token, sizeof(tokens[n_tokens]) - 1);
        tokens[n_tokens][sizeof(tokens[n_tokens]) - 1] = '\0';
        printf("token[%d] = '%s'\n", n_tokens, tokens[n_tokens]);
        n_tokens++;
        token = strtok(NULL, delim);
    }

    // trim – odstránenie bielych znakov zľava/pravá
    char line[64] = "  ahoj  \n";
    int len = (int)strlen(line);
    while (len > 0 && isspace((unsigned char)line[len - 1])) {
        line[--len] = '\0';
    }
    int start = 0;
    while (line[start] && isspace((unsigned char)line[start])) start++;
    printf("trim: '%s'\n", line + start);

    // užitočné funkcie: strchr/strstr/strnlen/memmove
    const char *p = strchr(s, 's');      // prvé 's' v reťazci (alebo NULL)
    printf("strchr('s') v \"%s\" -> %s\n", s, p ? p : "(NULL)");

    const char *sub = strstr(s, "svet"); // podreťazec
    printf("strstr(\"svet\") -> %s\n", sub ? sub : "(NULL)");

    size_t k = strnlen(s, 6);            // dĺžka max do 6
    printf("strnlen(s,6) = %zu\n", k);

    // memmove – bezpečný presun aj pri prekrývaní
    char overl[32] = "ABCDEF";
    memmove(overl + 2, overl, 4); // (prekrýva sa)
    overl[6] = '\0';
    printf("memmove overlap: %s\n", overl);
}

// ------------------------------
// 1b) sprintf/snprintf a SS CANF – parsovanie textu
// ------------------------------
static void demo_format_parse(void) {
    printf("\n=== sprintf/snprintf & sscanf ===\n");

    // sprintf vs snprintf
    char buf1[16], buf2[16];
    int a = 123, b = 456;

    // sprintf – NEKONTROLUJE dĺžku (iba ukážka)
    sprintf(buf1, "a=%d b=%d", a, b); // potenciálne nebezpečné
    printf("sprintf: '%s'\n", buf1);

    // snprintf – bezpečné, vráti počet znakov (bez nulového ukončenia)
    int n = snprintf(buf2, sizeof(buf2), "a=%d b=%d", a, b);
    printf("snprintf: '%s' (n=%d, cap=%zu)\n", buf2, n, sizeof(buf2));

    // sscanf – vyparsuj hodnoty z textu
    const char *line = "x=17 y=003 z=-5 name=David";
    int x, y, z;
    char name[32];
    int got = sscanf(line, "x=%d y=%d z=%d name=%31s", &x, &y, &z, name);
    if (got == 4) {
        printf("sscanf OK -> x=%d y=%d z=%d name=%s\n", x, y, z, name);
    } else {
        printf("sscanf neuspel, got=%d\n", got);
    }

    // strtol/strtoul – bezpečnejšie než atoi (kontrolujú chyby a koniec)
    const char *numtxt = "  1234xyz";
    char *end = NULL;
    long val = strtol(numtxt, &end, 10); // base 10
    printf("strtol: val=%ld, end=\"%s\"\n", val, end ? end : "(NULL)");
}

// ------------------------------
// 2) POLIA
// ------------------------------
static void demo_arrays(void) {
    printf("\n=== POLIA (C) ===\n");

    int a[5] = {1, 2, 3, 4, 5};
    int sum = 0;
    for (int i = 0; i < 5; i++) sum += a[i];
    printf("sucet = %d\n", sum);

    int b[10];
    for (int i = 0; i < 10; i++) b[i] = i * i;
    printf("b: ");
    for (int i = 0; i < 10; i++) printf("%d ", b[i]);
    printf("\n");

    // pole reťazcov (2D char pole)
    char mena[3][20] = {"David", "Katka", "Noemi"};
    for (int i = 0; i < 3; i++) {
        printf("meno[%d] = %s\n", i, mena[i]);
    }
}

// ------------------------------
// 2b) Dynamická pamäť (malloc/calloc/realloc/free)
// ------------------------------
static void demo_heap_memory(void) {
    printf("\n=== DYNAMICKA PAMAT (malloc/calloc/realloc/free) ===\n");

    // malloc – neinitializuje pamäť
    int *arr = (int*)malloc(5 * sizeof(int));
    if (!arr) { die("malloc"); return; }
    for (int i = 0; i < 5; ++i) arr[i] = i + 1;

    // calloc – inicializuje nulami
    int *zeros = (int*)calloc(5, sizeof(int));
    if (!zeros) { free(arr); die("calloc"); return; }

    // realloc – zmena veľkosti (môže presunúť blok)
    int *bigger = (int*)realloc(arr, 10 * sizeof(int));
    if (!bigger) { free(arr); free(zeros); die("realloc"); return; }
    for (int i = 5; i < 10; ++i) bigger[i] = i + 1;

    printf("bigger: ");
    for (int i = 0; i < 10; ++i) printf("%d ", bigger[i]);
    printf("\n");

    free(bigger);
    free(zeros);
}

// ------------------------------
// 3) SÚBORY – stdio (FILE*)
//    a) čítanie riadok po riadku
//    b) zápis (append)
//    c) getline (POSIX) – pohodlné čítanie riadku ľub. dĺžky
// ------------------------------
static void demo_files_stdio(void) {
    printf("\n=== SUBORY (FILE*) – citanie riadkov ===\n");

    // vytvor testovací súbor
    {
        FILE* wf = fopen("demo_stdio.txt", "w");
        if (!wf) { die("fopen(demo_stdio.txt, w)"); return; }
        fputs("Prvy riadok\nDruhy riadok\nTretí riadok\n", wf);
        fclose(wf);
    }

    // čítanie po riadkoch
    FILE* f = fopen("demo_stdio.txt", "r");
    if (!f) { die("fopen(demo_stdio.txt, r)"); return; }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        printf("[riadok] %s", line); // fgets ponechá '\n'
    }
    fclose(f);

    // append
    FILE* fa = fopen("demo_stdio.txt", "a");
    if (!fa) { die("fopen(demo_stdio.txt, a)"); return; }
    fputs("Doplneny koniec\n", fa);
    fclose(fa);

    // getline – načíta dynamicky (POSIX). Uloží aj '\n' (ak je).
    printf("\n[getline]\n");
    FILE* fr = fopen("demo_stdio.txt", "r");
    if (!fr) { die("fopen demo for getline"); return; }
    char *dyn = NULL;
    size_t cap = 0;
    ssize_t rn;
    while ((rn = getline(&dyn, &cap, fr)) != -1) {
        printf("len=%zd | %s", rn, dyn);
    }
    free(dyn);
    fclose(fr);

    printf("-> hotovo, skontroluj 'demo_stdio.txt'\n");
}

// ------------------------------
// 4) SÚBORY – POSIX open/read/write
//    a) čítanie po blokoch a výpis na stdout
//    b) zápis raw binárnych dát
//    c) lseek (posun v súbore)
// ------------------------------
static void demo_files_posix_read_chunks(void) {
    printf("\n=== SUBORY (POSIX) – read po kusoch ===\n");

    // vytvor testovací text
    {
        int fdw = open("demo_posix.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fdw < 0) { die("open(demo_posix.txt, W|C|T)"); return; }
        const char* txt = "Lorem ipsum dolor sit amet,\nconsectetur adipiscing elit.\n";
        ssize_t want = (ssize_t)strlen(txt);
        if (write(fdw, txt, (size_t)want) != want) die("write(demo_posix.txt)");
        close(fdw);
    }

    // čítanie po kusoch + výpis na stdout (fd=1)
    int fd = open("demo_posix.txt", O_RDONLY);
    if (fd < 0) { die("open(demo_posix.txt, R)"); return; }

    char buf[16];
    ssize_t n;
    printf("Obsah (chunky=16B):\n");
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        if (write(1, buf, (size_t)n) < 0) { die("write(stdout)"); close(fd); return; }
    }
    if (n < 0) die("read(demo_posix.txt)");
    close(fd);
}

static void demo_files_posix_write_raw(void) {
    printf("\n=== SUBORY (POSIX) – zapis raw bajtov ===\n");

    int fd = open("raw.bin", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { die("open(raw.bin)"); return; }

    int values[10];
    for (int i = 0; i < 10; i++) values[i] = i + 1;

    ssize_t want = (ssize_t)sizeof(values);
    if (write(fd, values, (size_t)want) != want) die("write(raw.bin)");
    close(fd);

    printf("-> zapisane %zu bajtov do 'raw.bin'\n", sizeof(values));
}

static void demo_files_posix_lseek(void) {
    printf("\n=== POSIX – lseek (posun v subore) ===\n");

    int fd = open("seek_demo.bin", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { die("open seek_demo.bin"); return; }

    // zapíšeme 1 bajt, potom posun o 1024 (vytvorí "dieru"), potom ďalší bajt
    unsigned char one = 0xAB, two = 0xCD;
    if (write(fd, &one, 1) != 1) { die("write one"); close(fd); return; }
    if (lseek(fd, 1024, SEEK_CUR) < 0) { die("lseek"); close(fd); return; }
    if (write(fd, &two, 1) != 1) { die("write two"); close(fd); return; }
    close(fd);

    printf("-> Vznikol sparse file s dierou ~1024 bajtov.\n");
}

// ------------------------------
// 5) PRINTF – formáty
// ------------------------------
static void demo_printf_formats(void) {
    printf("\n=== PRINTF – formaty ===\n");
    int i = 42;
    long L = 9000000000L;
    double d = 3.14159;
    const char* s = "text";

    printf("int: %d, hex: 0x%x, oct: %o\n", i, (unsigned)i, (unsigned)i);
    printf("long: %ld\n", L);
    printf("double: %.3f (3 des. miesta)\n", d);
    printf("string: %s, znak: %c\n", s, 'A');
    printf("sirka a zarovnanie: |%10s| |%-10s|\n", "vpravo", "vlavo");
    printf("precision retezec: |%.5s|\n", "dlhy_retezec");
}

// ------------------------------
// 6) RÝCHLY PREHĽAD: bezpečný zápis na fd (ošetrenie EINTR/partial write)
// ------------------------------
static ssize_t write_all(int fd, const void *buf, size_t len) {
    const unsigned char *p = (const unsigned char*)buf;
    size_t left = len;
    while (left) {
        ssize_t w = write(fd, p, left);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        p += w;
        left -= (size_t)w;
    }
    return (ssize_t)len;
}

static void demo_write_all(void) {
    printf("\n=== write_all – ukazka ===\n");
    int fd = open("all.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { die("open all.txt"); return; }
    const char *msg = "Bezpecny zapis celeho buffera.\n";
    if (write_all(fd, msg, strlen(msg)) < 0) die("write_all");
    close(fd);
    printf("-> zapisane do all.txt\n");
}

// ------------------------------
// MAIN – spusti, čo potrebuješ trénovať
// ------------------------------
int main(void) {
    demo_strings();
    demo_format_parse();
    demo_arrays();
    demo_heap_memory();
    demo_files_stdio();
    demo_files_posix_read_chunks();
    demo_files_posix_write_raw();
    demo_files_posix_lseek();
    demo_printf_formats();
    demo_write_all();

    printf("\nHotovo. 👌\n");
    return 0;
}