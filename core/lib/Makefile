#
# Makefile for Core functions
# (by Randal A. Koene, randalk@minduploading.org)
#

SHELL=/usr/bin/bash

##########################################################
## Compiler
## --------
CC=gcc
CCPP=g++ 
##########################################################

FORMALIZERPATH=$(HOME)/src/formalizer
COREINCLUDEPATH=$(FORMALIZERPATH)/core/include
CORELIBPATH=$(FORMALIZERPATH)/core/lib

INC=$(COREINCLUDEPATH)
OBJ=./obj
TEST=./test

#INCLUDES= -I$(HOME)/src/include
# to find the postgresql header files (e.g libpq-fe.h), do this: pg_config --includedir
INCLUDES= -I$(CPATH) -I$(COREINCLUDEPATH) -I/usr/include/postgresql

# to find the postgresql library files fo this: pg_config --libdir
LIB_PATH= -L/usr/lib/x86_64-linux-gnu -lpq

##########################################################
## Machine selection
## -----------------
## aurora (x86):
MACHSPEC=
GCCTUNE=$(if $(shell gcc -v 2>&1 | grep "^gcc version[^0-9]*[4-9][.]"),-mtune=pentium4)
MACHOPT=
#MACHOPT=$(GCCTUNE) -mfpmath=sse
##
##
##########################################################

### Attempting a temporary fix for gcc 4.x compilation of dil2al
### (This should be replaced by actual source code modification.)
### Also, attempting a temporary fix for gcc 7.1+ compilation of dil2al
### (This should also be replaced by actual source code modification.)
CPPTEMPWORKAROUND=-fno-access-control -Wno-format-truncation

GCC3FXTRA=$(if $(shell gcc -v 2>&1 | grep "^gcc version[^0-9]*[3-9][.]"),-Wno-unused-function)
CFXTRA= -DSTDC_HEADERS -pedantic -Wall -Werror -Wno-char-subscripts $(GCC3FXTRA)

## a few C++ constructs that are new to C++20, such as designated initializer lists,
## so let's add support for that here:
CPP2020= -std=gnu++2a
CPPXTRA= $(MACHOPT) -mieee-fp -ffast-math -DDEFAULTHOMEDIR=\"$(HOME)\" -pedantic -Wall -Werror $(GCC3FXTRA) $(CPPTEMPWORKAROUND) $(CPP2020)
##
##########################################################

##########################################################
## Compiler Options
## ----------------
## debugging information (updates 2019-02-01):
#COMPOPT= -g -Og
#OPTSTR=editing_optimized_for_debugging
## generate profile information for use with gprof:
#COMPOPT= -g -O6
#OPTSTR=profiling
## optimized (USUALLY USE THIS ONE!):
COMPOPT= -O3
OPTSTR=optimized
## optimized + debugging info (as per GDB recommendations 2019-02-01)
#COMPOPT= -g -O3
#OPTSTR=production_optimized_with_dbg_symbols
##########################################################

##########################################################
## C++ Specific Compiler Options
## -----------------------------
## debugging information:
#CPPOPT=
## generate profile information for use with gprof:
#CPPOPT=
## optimized:
CPPOPT= -felide-constructors
## uncomment the following optimization option only if
## you have an older C++ compiler that does not optimize
## return values unless explicitly told to do so with
## the now deprecated `named return value' extension:
#CPPOPT= $(CPPOPT) -D_USE_NAMED_RETURN_VALUE_EXTENSION
##########################################################

CFLAGS= $(COMPOPT) $(MACHSPEC) $(CFXTRA)
CPPFLAGS= $(COMPOPT) $(CPPOPT) $(MACHSPEC) $(CPPXTRA) $(INCLUDES)

.PHONY: FORCE

all: init $(OBJ)/general.o $(OBJ)/error.o $(OBJ)/TimeStamp.o $(OBJ)/Graphtypes.o $(OBJ)/Graphpostgres.o

init: FORCE
	@echo '-------------------------------------------------------------------'
	@echo 'Compilation options: $(MACHSTR), $(OPTSTR)'
	@echo 'Includes: $(CPPFLAGS)'
	@echo '-------------------------------------------------------------------'

$(OBJ)/error.o: error.cpp $(INC)/error.hpp
	$(CCPP) $(CPPFLAGS) -c error.cpp -o $(OBJ)/error.o

$(OBJ)/general.o: general.cpp $(INC)/general.hpp $(INC)/error.hpp $(OBJ)/error.o
	$(CCPP) $(CPPFLAGS) -c general.cpp -o $(OBJ)/general.o

$(OBJ)/TimeStamp.o: TimeStamp.cpp $(INC)/TimeStamp.hpp $(INC)/error.hpp $(OBJ)/error.o
	$(CCPP) $(CPPFLAGS) -c TimeStamp.cpp -o $(OBJ)/TimeStamp.o

$(OBJ)/Graphtypes.o: Graphtypes.cpp $(INC)/Graphtypes.hpp $(INC)/general.hpp $(OBJ)/general.o
	$(CCPP) $(CPPFLAGS) -c Graphtypes.cpp -o $(OBJ)/Graphtypes.o

$(OBJ)/Graphpostgres.o: Graphpostgres.cpp $(INC)/Graphpostgres.hpp $(INC)/Graphtypes.hpp \
						$(INC)/general.hpp $(INC)/error.hpp $(OBJ)/Graphtypes.o \
						$(OBJ)/general.o $(OBJ)/error.o
	$(CCPP) $(CPPFLAGS) -c Graphpostgres.cpp -o $(OBJ)/Graphpostgres.o

clean:
	rm -f $(OBJ)/*.o $(TEST)/*.o

#doc++:
#	rm -r -f html
#	doc++ -d html spiker.dxx
