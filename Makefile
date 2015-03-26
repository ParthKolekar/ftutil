OBJ = ftutil
override CFLAGS += -Ofast -funroll-loops -flto -Wall -DCOLOR
override DEVEL_FLAGS += -g -Wall -DCOLOR
override LFLAGS += 
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
CC = gcc

$(OBJ) : main.o 
	$(CC) -Wall $^ -o $@ $(LFLAGS)

%.o : %.c
	$(CC) $(DEVEL_FLAGS) -c $^

clean :
	rm -f *.o $(OBJ)

ofast :
	$(CC) $(CFLAGS) -c main.c
	$(CC) -Wall *.o -o $(OBJ) $(LFLAGS) $(CFLAGS)

install : $(OBJ)
	@echo "Installing program to $(DESTDIR)$(BINDIR) ..."
	@mkdir -p $(DESTDIR)$(BINDIR)
	@install -pm0755 $(OBJ) $(DESTDIR)$(BINDIR)/$(TARGET) || \
		echo "Failed. Try "make PREFIX=~ install" ?"
