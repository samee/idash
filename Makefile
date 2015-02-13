all: hamming

idash_common: idash_common.cpp
	g++ -o $@ -O3 --std=c++11 $?

CILPATH=~/Projects/frontend/cil
UTILPATH=$(CILPATH)/test/oblivc
UTILSRC=$(UTILPATH)/common/util.c
OBLIVCC=$(CILPATH)/bin/oblivcc
CFLAGS=-O3
hamming: hamming.c hamming.h
	$(OBLIVCC) -o $@ $(CFLAGS) -I . hamming.c
