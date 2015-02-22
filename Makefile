all: hamming chi2

idash_common: idash_common.cpp
	g++ -o $@ -O3 --std=c++11 $?

CILPATH=~/Projects/frontend/cil
UTILPATH=$(CILPATH)/test/oblivc
UTILSRC=$(UTILPATH)/common/util.c
OBLIVCC=$(CILPATH)/bin/oblivcc
CFLAGS=-O3 -DNDEBUG
hamming: hamming.c hamming.h psi.oc util.c util.h
	$(OBLIVCC) -o $@ $(CFLAGS) -I . hamming.c psi.oc util.c

chi2: chi2.c chi2.oc chi2.h util.c util.h
	$(OBLIVCC) -o $@ $(CFLAGS) -I . chi2.c chi2.oc util.c
