# OSY – C CHEAT SHEET (IBA TO, ČO POTREBUJEŠ NA SKÚŠKU)

---

## 1. STRINGY & PAMÄŤ (NEVYHNUTNÉ)

```c
memset(buf, 0, size);              // vymazanie pamäte
memcpy(dst, src, n);               // kopírovanie bajtov (NEKONČÍ \0)
strlen(str);                       // dĺžka stringu bez \0
strcmp(a, b);                      // porovnanie stringov
strncmp(a, b, n);                  // porovnanie n znakov
snprintf(buf, size, "%s", str);  // bezpečné skladanie stringu
```

⚠️ Nikdy nepoužívaj `strcpy`, ak si nie si 100 % istý veľkosťou.

---

## 2. VSTUP / VÝSTUP (SÚBORY, SOCKETY)

```c
int fd = open("file.png", O_RDONLY);
int n = read(fd, buf, sizeof(buf));
write(fd, buf, n);
close(fd);
```

⚠️ `read()` môže vrátiť:

* `>0` počet bajtov
* `0` EOF
* `-1` chyba

---

## 3. SOCKET SERVER – KOSTRA (MUSÍŠ VEDIEŤ PORADIE)

```c
socket(AF_INET, SOCK_STREAM, 0);
bind(sock, ...);
listen(sock, 1);
accept(sock, ...);
```

Klient:

```c
socket(AF_INET, SOCK_STREAM, 0);
connect(sock, ...);
```

---

## 4. VLÁKNA (THREAD SERVER)

```c
pthread_t t;
pthread_create(&t, NULL, client_handle, arg);
pthread_detach(t);   // server

void *client_handle(void *arg) {
    int sock = *(int*)arg;
    free(arg);
    return NULL;
}
```

⚠️ Argument DO threadu vždy ako pointer!

---

## 5. PROCESY (KLIENT – DISPLAY OBRÁZKU)

```c
pid_t pid = fork();
if (pid == 0) {
    execvp("display", argv);
    exit(1);
}
wait(NULL);
```

---

## 6. SEMAFORY (KĽÚČ K ZADANIU)

```c
sem_t sem[4];
sem_init(&sem[i], 0, 1);

sem_wait(&sem[i]);   // DOWN
// kritická sekcia
sem_post(&sem[i]);   // UP
```

Pomenované:

```c
sem_open("/sem0", O_CREAT, 0666, 1);
```

---

## 7. PARSOVANIE DÁTUMU (DAY DD.MM)

```c
int day, month;
sscanf(line, "DAY %d.%d", &day, &month);
```

Alebo manuálne:

```c
memcpy(buf, line+4, 2);
buf[2] = '\0';
```

---

## 8. DEŇ V ROKU (0–364)

```c
int dim[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
int d = 0;
for (int i = 0; i < month-1; i++) d += dim[i];
d += day - 1;
```

---

## 9. ROČNÉ OBDOBIE → INDEX

```text
0–79   = zima   (3)
80–171 = jar    (0)
172–263= leto   (1)
264–364= jeseň  (2)
```

---

## 10. POMALÉ POSIELANIE DÁT

```c
write(sock, buf, n);
usleep(50000);  // 50 ms
```

---

## 11. DEBUG & CHYBY

```c
perror("chyba");
errno;
strerror(errno);
```

---

## 🔥 SKÚŠKOVÝ MINIMUM CHECKLIST

* [ ] socket → bind → listen → accept
* [ ] read / write bez prepisu pamäte
* [ ] pthread_create + správny argument
* [ ] sem_wait / sem_post podľa indexu
* [ ] sscanf("DAY %d.%d")
* [ ] fork + execvp + wait
* [ ] usleep pri posielaní

---

Toto je **všetko**, čo potrebuješ. Nič navyše.
