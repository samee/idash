all: hamming chi2

idash_common: idash_common.cpp
	g++ -o $@ -O3 --std=c++11 $?

OCPATH=~/Projects/frontend/cil
UTILPATH=$(OCPATH)/test/oblivc
UTILSRC=$(UTILPATH)/common/util.c
OBLIVCC=$(OCPATH)/bin/oblivcc
CFLAGS=-O3 -DNDEBUG
hamming: hamming.c hamming.h psi.oc util.c util.h
	$(OBLIVCC) -o $@ $(CFLAGS) -I . hamming.c psi.oc util.c

chi2: chi2.c chi2.oc chi2.h util.c util.h
	$(OBLIVCC) -o $@ $(CFLAGS) -I . chi2.c chi2.oc util.c

hamming2: hamming2.c hamming.h util.c
	$(OBLIVCC) -o $@ $(CFLAGS) -I . hamming2.c util.c

clean:
	rm -f *.oc.c *.i hamming hamming2 chi2
