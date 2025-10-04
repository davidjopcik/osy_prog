# OSY – Makefile Helper (krátka verzia)

## Základné pojmy
- **Cieľ (target)**: výsledok (napr. `app`, `clean`, `lib`).
- **Závislosti (deps)**: súbory, čo musia byť hotové pred cieľom.
- **Príkazy (recipes)**: shell príkazy (riadky musia začínať **TAB**-om).
- **Premenné**: `CC`, `CFLAGS`, `LDFLAGS`, `LDLIBS`, `SRCS`, `OBJS`…

## 1) Najmenší Makefile (jeden .c súbor)
```Makefile
# súbor: main.c -> app
app: main.c
	gcc -Wall -Wextra -O2 main.c -o app

clean:
	rm -f app
```

## 2) Viac súborov (.c -> .o -> app)
```Makefile
CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -std=c11
LDFLAGS :=
LDLIBS  :=    # napr. -lm

SRCS := main.c foo.c bar.c
OBJS := $(SRCS:.c=.o)
APP  := app

$(APP): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean run
clean:
	rm -f $(OBJS) $(APP)

run: $(APP)
	./$(APP)
```

## 3) Štruktúra `src/`, `include/`, `build/`, `bin/`
```Makefile
CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -std=c11 -Iinclude
SRCS    := $(wildcard src/*.c)
OBJS    := $(patsubst src/%.c, build/%.o, $(SRCS))
APP     := bin/app

$(APP): $(OBJS) | bin
	$(CC) -o $@ $(OBJS)

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

bin build:
	mkdir -p $@

.PHONY: clean
clean:
	rm -rf build bin
```

## 4) Statická knižnica (`libmylib.a`) + aplikácia
```Makefile
CC      := gcc
AR      := ar
CFLAGS  := -Wall -Wextra -O2 -Iinclude

LIB_OBJS := foo.o bar.o
LIB_NAME := libmylib.a
APP_OBJS := main.o
APP      := app

all: $(LIB_NAME) $(APP)

$(LIB_NAME): $(LIB_OBJS)
	$(AR) rcs $@ $^

$(APP): $(APP_OBJS) $(LIB_NAME)
	$(CC) -o $@ $(APP_OBJS) -L. -lmylib

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(LIB_NAME) $(APP) $(LIB_OBJS) $(APP_OBJS)
```

## 5) Dynamická knižnica (`libmylib.so`) + beh s `LD_LIBRARY_PATH`
```Makefile
CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -fPIC -Iinclude
LDFLAGS := -shared

LIB_OBJS := foo.o bar.o
LIB_SO   := libmylib.so
APP      := app

all: $(LIB_SO) $(APP)

$(LIB_SO): $(LIB_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(APP): main.o $(LIB_SO)
	$(CC) -o $@ main.o -L. -lmylib

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(APP)
	export LD_LIBRARY_PATH=.:$$LD_LIBRARY_PATH; ./$(APP)

.PHONY: clean
clean:
	rm -f $(APP) $(LIB_SO) $(LIB_OBJS) main.o
```

## 6) Debug vs Release (prepínanie profilov)
```Makefile
# Použi: make MODE=debug  alebo  make MODE=release
MODE ?= debug
CFLAGS_COMMON := -std=c11 -Iinclude -Wall -Wextra
CFLAGS_debug  := $(CFLAGS_COMMON) -O0 -g -DDEBUG
CFLAGS_release:= $(CFLAGS_COMMON) -O2 -DNDEBUG
CFLAGS := $(CFLAGS_$(MODE))

app: main.o
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f *.o app
```

## Užitočné tipy
- **TAB** je povinný v recepte (nie medzery).
- Automatické premenné: `$@` (cieľ), `$<` (prvá závislosť), `$^` (všetky závislosti).
- `.PHONY` pre ciele bez súborov (clean, run, all).
- `make -j` urýchli build (paralelne).
- `ldd ./app` ukáže dynamické závislosti.
- `nm -C library.a | grep symbol` – kontrola symbolov.
- Pre C++: nahraď `CC:=g++` a pridaj `-std=c++17`.
