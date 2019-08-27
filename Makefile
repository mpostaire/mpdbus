SDIR=src
ODIR=out
CFLAGS=`pkg-config --cflags gio-2.0 gio-unix-2.0 glib-2.0 libmpdclient` -g
LDLIBS=`pkg-config --libs gio-2.0 gio-unix-2.0 glib-2.0 libmpdclient`
CC=gcc
EXEC=mpdbus
SRC= $(wildcard $(SDIR)/*.c)
OBJ= $(SRC:$(SDIR)/%.c=$(ODIR)/%.o)

all: out $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDLIBS)

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -o $@ -c $< $(CFLAGS) $(LDLIBS)

out:
	mkdir $@

run: all
	./$(EXEC)

install: all
	install -m 0755 $(EXEC) /usr/bin/

uninstall:
	rm -f /usr/bin/$(EXEC)

clean:
	rm -f $(OBJ)

cleaner: clean
	rm -f $(EXEC)

.PHONY: all clean cleaner install uninstall run
