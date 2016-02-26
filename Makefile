OBJ = ftutil
override CFLAGS += -O3 -funroll-loops -flto -Wall -DCOLOR
override LFLAGS += 
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
CC = gcc

$(OBJ) : main.o 
	$(CC) -Wall $^ -o $@ $(LFLAGS) $(CFLAGS)

main.o : main.c
	$(CC) $(CFLAGS) -c $^

clean :
	rm -f main.o $(OBJ)

install : $(OBJ)
	@echo "Installing program to $(DESTDIR)$(BINDIR) ..."
	@mkdir -p $(DESTDIR)$(BINDIR)
	@install -pm0755 $(OBJ) $(DESTDIR)$(BINDIR)/$(TARGET) || \
		echo "Failed. Try "make PREFIX=~ install" ?"
