OBJ = ftutil
override CFLAGS += -O3 -flto -Wall -DCOLOR
override LFLAGS += 
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
CC = gcc

$(OBJ) : main.c
	$(CC) -Wall $^ -o $@ $(LFLAGS) $(CFLAGS)

clean :
	rm -f main.o $(OBJ)

debug: main.c
	rm -f main.o $(OBJ)
	$(CC) -O0 -g -DCOLOR $^ -o ${OBJ}

install : $(OBJ)
	@echo "Installing program to $(DESTDIR)$(BINDIR) ..."
	@mkdir -p $(DESTDIR)$(BINDIR)
	@install -pm0755 $(OBJ) $(DESTDIR)$(BINDIR)/$(TARGET) || \
		echo "Failed. Try "make PREFIX=~ install" ?"
