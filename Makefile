PREFIX = /usr/local
VERSION = 1-2-0

#this must be built 32 bit
CFLAGS:=$(CFLAGS) -m32
LDFLAGS:=$(LDFLAGS) -m32 -framework ApplicationServices

SRC = Makefile pict2pdf.c
DOC = README 
TEST = sample.pict sample.orig.pdf
OBJS = pict2pdf.o
PROG = pict2pdf
MANPAGE = pict2pdf.1
DISTNAME = $(PROG)-$(VERSION)
HTMLDOC = pict2pdf.html

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(PROG)

html $(HTMLDOC): 
	install -d $(PREFIX)/share/man/man1 
	install -m 0644 $(MANPAGE) $(PREFIX)/share/man/man1
	man pict2pdf | man2html > /tmp/index.html
	perl -pe "s@(http.*f/)@<a href=\"\1\"> \1</a>@" /tmp/index.html > $(HTMLDOC)
	rm /tmp/index.html

test: sample.pict $(PROG)
	./$(PROG) -f -r 10 -s 0.75 sample.pict
	
install: $(PROG) $(MANPAGES)
	install -d $(PREFIX)/bin 
	install $(PROG) $(PREFIX)/bin 
	install -d $(PREFIX)/share/man/man1 
	install -m 0644 $(MANPAGE) $(PREFIX)/share/man/man1

dist: $(SRC) $(MANPAGES) $(DOC) $(TEST)
	mkdir $(DISTNAME)
	ln $(SRC)      $(DISTNAME)
	ln $(DOC)      $(DISTNAME)
	ln $(TEST)     $(DISTNAME)
	ln $(MANPAGE) $(DISTNAME)
#	tar cvf - $(DISTNAME) | gzip > $(DISTNAME).tar.gz
	zip -r $(DISTNAME) $(DISTNAME)
	rm -rf $(DISTNAME)

appleclean:
	sudo xattr -r -d com.apple.FinderInfo ./
	sudo xattr -r -d com.apple.TextEncoding ./
	sudo xattr -r -d com.apple.quarantine ./
	
clean:
	rm -rf $(PROG) $(OBJS) $(HTMLDOC) sample.pdf
	rm -rf $(DISTNAME)
