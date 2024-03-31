#
# Students' Makefile for the Malloc Lab
#
TEAM = david61song
VERSION = 1
HANDINDIR = handme
CC = clang
CFLAGS = -Wall -arch x86_64 -Wno-unused-function -Wno-unused-parameter

UTILS = mdriver.o memlib.o fsecs.o fcyc.o clock.o ftimer.o

NAIVE = mm-naive.o
EXPLICIT = mm-explicit.o
# NOT YET IMPLEMENTED
# SEGREGATED = mm-segregated.o
# BUDDY = mm-buddy.o
# glibc
# tcmalloc ..


naive: $(UTILS) $(NAIVE)
	$(CC) $(CFLAGS) -o mdriver $(UTILS) $(NAIVE)
explicit : $(UTILS) $(EXPLICIT)
	$(CC) $(CFLAGS) -o mdriver $(UTILS) $(EXPLICIT)


# Magical things happens
#
mdriver.o: mdriver.c fsecs.h fcyc.h clock.h memlib.h config.h mm.h
memlib.o: memlib.c memlib.h
fsecs.o: fsecs.c fsecs.h config.h
fcyc.o: fcyc.c fcyc.h
ftimer.o: ftimer.c ftimer.h config.h
clock.o: clock.c clock.h
mm-naive.o : mm-naive.c mm.h memlib.h
mm-explicit.o : mm-explicit.c mm.h memlib.h

handin:
	for file in mm-*.c; do \
		cp $$file $(HANDINDIR)/$(TEAM)-$(VERSION)-$$file; \
	done

clean:
	rm -f *~ *.o mdriver



