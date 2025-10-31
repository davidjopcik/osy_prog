// osy_cheatsheet.c
// Základy v C: stringy, polia, súbory (stdio + POSIX), printf, read/write.
// Kompilácia: gcc -std=c17 -Wall -Wextra -O2 osy_cheatsheet.c -o osy

#include <stdio.h>      // printf, FILE*, fopen, fgets, fputs, fclose
#include <string.h>     // strlen, strcpy, strncpy, strcat, strcmp, strtok, memset, strerror
#include <ctype.h>      // isspace
#include <errno.h>      // errno
#include <stdlib.h>     // exit, malloc, free
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>      // open
#include <unistd.h>     // read, write, close

static void die(const char* msg) {
    printf("[ERR] %s: %s\n", msg, strerror(errno));
}

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

    // bezpečné skladanie do buffera (snprintf)
    char joined[128];
    snprintf(joined, sizeof(joined), "%s %d %s", "cislo", 42, "OK");
    printf("snprintf -> '%s'\n", joined);

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
// 3) SÚBORY – stdio (FILE*)
//    a) čítanie riadok po riadku
//    b) zápis (append)
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

    printf("-> hotovo, skontroluj 'demo_stdio.txt'\n");
}

// ------------------------------
// 4) SÚBORY – POSIX open/read/write
//    a) čítanie po blokoch a výpis na stdout
//    b) zápis raw binárnych dát
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
}

// ------------------------------
// MAIN – spusti, čo potrebuješ trénovať
// ------------------------------
int main(void) {
    demo_strings();
    demo_arrays();
    demo_files_stdio();
    demo_files_posix_read_chunks();
    demo_files_posix_write_raw();
    demo_printf_formats();

    printf("\nHotovo. 👌\n");
    return 0;
}