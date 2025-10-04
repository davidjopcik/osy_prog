CC = gcc
CFLAGS = -O2 -Wall -fPIC

BIN         = verbank
ACC_DIR     = rclib
RC_DIR      = verlib

SHLIB_ACC   = $(ACC_DIR)/libver.so
SHLIB_RC    = $(RC_DIR)/libver.so

all: $(SHLIB_ACC) $(SHLIB_RC) $(BIN)

$(ACC_DIR)/ver.o: $(ACC_DIR)/ver.c $(ACC_DIR)/ver.h
	$(CC) $(CFLAGS) -c $(ACC_DIR)/ver.c -o $(ACC_DIR)/ver.o

$(SHLIB_ACC): $(ACC_DIR)/ver.o
	$(CC) -shared -o $(SHLIB_ACC) $(ACC_DIR)/ver.o

$(RC_DIR)/ver.o: $(RC_DIR)/ver.c $(RC_DIR)/ver.h
	$(CC) $(CFLAGS) -c $(RC_DIR)/ver.c -o $(RC_DIR)/ver.o

$(SHLIB_RC): $(RC_DIR)/ver.o
	$(CC) -shared -o $(SHLIB_RC) $(RC_DIR)/ver.o

$(BIN): verbank.c $(RC_DIR)/ver.h
	$(CC) -O2 -Wall verbank.c -L$(RC_DIR) -lver -o $(BIN)


clean:
	rm -f $(ACC_DIR)/*.o $(RC_DIR)/*.o $(SHLIB_ACC) $(SHLIB_RC) $(BIN)