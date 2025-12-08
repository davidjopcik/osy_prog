
TARGET = gennum
LIBNAME = gen ver
LIBFILE = lib$(LIBNAME).a

CPPFLAGS += -g
LDFLAGS += -L. 
LDLIBS += -l$(LIBNAME)

all: $(TARGET)

# clean binary files
clean:
	rm -rf $(TARGET) $(LIBFILE) *.o

# creating static library 
$(LIBFILE): gen.o 
	@echo "\nCreating static library...\n"
	#rm -f $@
	ar r $@ $^
	@echo "\nStatic library created: '$(LIBFILE)'.\n"

# creating dynamic library 
$(LIBFILE): ver.o 
	@echo "\nCreating dynamic library...\n"
	gcc $(CPPFLAGS) $^ -shared $(LDFLAGS) -o $@
	@echo "\nDynamic library created: '$(LIBFILE)'.\n"

# creating application
$(TARGET): $(TARGET).c $(LIBFILE)
	@echo "\nCreating application...\n"
	gcc $(CFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@
	@echo "\nApplication '$(TARGET)' is ready.\n"

