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

##########################################################
## Compiler Options
## ----------------
## debugging information (updates 2019-02-01):
#COMPOPT= -g -O0
#OPTSTR=optimized_for_debugging
## generate profile information for use with gprof:
#COMPOPT= -g -O6
#OPTSTR=profiling
## optimized (USUALLY USE THIS ONE!):
COMPOPT= -O3
OPTSTR=optimized
##########################################################

CONFIGROOT=-DCONFIG_ROOT=\"$(HOME)/.formalizer/config\"
DEFAULTDBNAME=-DDEFAULT_DBNAME=\"formalizer\"
DEFAULTSCHEMA=-DDEFAULT_PQ_SCHEMANAME=\"$(USER)\"

FORMALIZERPATH=$(HOME)/src/formalizer
COREINCLUDEPATH=$(FORMALIZERPATH)/core/include
CORELIBPATH=$(FORMALIZERPATH)/core/lib

INC=$(COREINCLUDEPATH)
OBJ=./obj
TEST=./test

# To find the Boost libraries
# See the note on POSIX systems in Building Boost.Interprocess at https://www.boost.org/doc/libs/1_38_0/doc/html/interprocess.html#interprocess.intro.introduction_building_interprocess.
BOOSTINTERPROCESSLIB_REQS = -lrt -lpthread
LOCALINC = $(HOME)/local/include
LOCALLIB = $(BOOSTINTERPROCESSLIB_REQS)

#INCLUDES= -I$(HOME)/src/include
# to find the postgresql header files (e.g libpq-fe.h), do this: pg_config --includedir
INCLUDES = -I$(CPATH) -I$(COREINCLUDEPATH) -I/usr/include/postgresql
INCLUDES += -I$(LOCALINC)

# Add this include directory for dilaccess.cpp and programs that use it.
# Using this links in a lot!
DIL2AL=$(HOME)/src/dil2al

# to find the postgresql library files fo this: pg_config --libdir
LIB_PATH = -L/usr/lib/x86_64-linux-gnu -lpq
LIB_PATH += $(POSTGRESLIB) $(LOCALLIB)

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
#CPPTEMPWORKAROUND=-fno-access-control -Wno-format-truncation

GCC3FXTRA=$(if $(shell gcc -v 2>&1 | grep "^gcc version[^0-9]*[3-9][.]"),-Wno-unused-function)
CFXTRA= -DSTDC_HEADERS -pedantic -Wall -Werror -Wno-char-subscripts $(GCC3FXTRA)

## a few C++ constructs that are new to C++20, such as designated initializer lists,
## so let's add support for that here:
CPP2020= -std=gnu++2a
CPPXTRA= $(MACHOPT) -mieee-fp -ffast-math -DDEFAULTHOMEDIR=\"$(HOME)\" -pedantic -Wall -Werror $(GCC3FXTRA) $(CPPTEMPWORKAROUND) $(CPP2020)
##
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

SOURCES = $(wildcard *.cpp)
OBJECTS = $(addprefix $(OBJ)/,$(SOURCES:%.cpp=%.o))

.PHONY: FORCE

all: init $(OBJECTS)

init: FORCE
	@echo '-------------------------------------------------------------------'
	@echo 'Compilation options: $(MACHSTR), $(OPTSTR)'
	@echo 'Includes: $(CPPFLAGS)'
	@echo 'Core sources: $(SOURCES)'
	@echo 'Core objects built here: $(OBJECTS)'
	@echo '-------------------------------------------------------------------'

$(OBJ)/error.o: error.cpp $(INC)/error.hpp
	$(CCPP) $(CPPFLAGS) -c error.cpp -o $(OBJ)/error.o

$(OBJ)/standard.o: standard.cpp $(INC)/standard.hpp $(INC)/error.hpp
	$(CCPP) $(CPPFLAGS) $(DEFAULTDBNAME) $(DEFAULTSCHEMA) -c standard.cpp -o $(OBJ)/standard.o

$(OBJ)/config.o: config.cpp $(INC)/config.hpp $(INC)/standard.hpp $(INC)/jsonlite.hpp $(INC)/error.hpp
	$(CCPP) $(CPPFLAGS) $(CONFIGROOT) $(DEFAULTDBNAME) $(DEFAULTSCHEMA) -c config.cpp -o $(OBJ)/config.o

$(OBJ)/general.o: general.cpp $(INC)/general.hpp
	$(CCPP) $(CPPFLAGS) -c general.cpp -o $(OBJ)/general.o

$(OBJ)/stringio.o: stringio.cpp $(INC)/error.hpp
	$(CCPP) $(CPPFLAGS) -c stringio.cpp -o $(OBJ)/stringio.o

$(OBJ)/proclock.o: proclock.cpp $(INC)/error.hpp $(INC)/TimeStamp.hpp
	$(CCPP) $(CPPFLAGS) -c proclock.cpp -o $(OBJ)/proclock.o

$(OBJ)/utf8.o: utf8.cpp $(INC)/utf8.hpp $(INC)/error.hpp
	$(CCPP) $(CPPFLAGS) -c utf8.cpp -o $(OBJ)/utf8.o

$(OBJ)/html.o: html.cpp $(INC)/html.hpp
	$(CCPP) $(CPPFLAGS) -c html.cpp -o $(OBJ)/html.o

$(OBJ)/jsonlite.o: jsonlite.cpp $(INC)/jsonlite.hpp $(INC)/general.hpp
	$(CCPP) $(CPPFLAGS) -c jsonlite.cpp -o $(OBJ)/jsonlite.o

$(OBJ)/templater.o: templater.cpp $(INC)/templater.hpp $(INC)/error.hpp
	$(CCPP) $(CPPFLAGS) -c templater.cpp -o $(OBJ)/templater.o

$(OBJ)/ReferenceTime.o: ReferenceTime.cpp $(INC)/ReferenceTime.hpp $(INC)/error.hpp
	$(CCPP) $(CPPFLAGS) -c ReferenceTime.cpp -o $(OBJ)/ReferenceTime.o

$(OBJ)/TimeStamp.o: TimeStamp.cpp $(INC)/TimeStamp.hpp $(INC)/error.hpp
	$(CCPP) $(CPPFLAGS) -c TimeStamp.cpp -o $(OBJ)/TimeStamp.o

$(OBJ)/dilaccess.o: dilaccess.cpp $(INC)/dilaccess.hpp $(INC)/error.hpp
	$(CCPP) $(CPPFLAGS) -I$(DIL2AL) -c dilaccess.cpp -o $(OBJ)/dilaccess.o

$(OBJ)/Graphcompare.o: Graphcompare.cpp $(INC)/Graphtypes.hpp
	$(CCPP) $(CPPFLAGS) -c Graphcompare.cpp -o $(OBJ)/Graphcompare.o

$(OBJ)/Graphbase.o: Graphbase.cpp $(INC)/Graphbase.hpp $(INC)/error.hpp $(INC)/TimeStamp.hpp
	$(CCPP) $(CPPFLAGS) -c Graphbase.cpp -o $(OBJ)/Graphbase.o

$(OBJ)/Graphtypes.o: Graphtypes.cpp $(INC)/Graphtypes.hpp $(INC)/general.hpp $(INC)/TimeStamp.hpp
	$(CCPP) $(CPPFLAGS) -c Graphtypes.cpp -o $(OBJ)/Graphtypes.o

$(OBJ)/Graphinfo.o: Graphinfo.cpp $(INC)/Graphinfo.hpp $(INC)/Graphtypes.hpp $(INC)/error.hpp
	$(CCPP) $(CPPFLAGS) -c Graphinfo.cpp -o $(OBJ)/Graphinfo.o

$(OBJ)/Graphunshared.o: Graphunshared.cpp $(INC)/Graphunshared.hpp $(INC)/general.hpp $(INC)/TimeStamp.hpp
	$(CCPP) $(CPPFLAGS) -c Graphunshared.cpp -o $(OBJ)/Graphunshared.o

$(OBJ)/LogtypesID.o: LogtypesID.cpp $(INC)/LogtypesID.hpp $(INC)/Graphtypes.hpp $(INC)/general.hpp $(INC)/TimeStamp.hpp
	$(CCPP) $(CPPFLAGS) -c LogtypesID.cpp -o $(OBJ)/LogtypesID.o

$(OBJ)/Logtypes.o: Logtypes.cpp $(INC)/Logtypes.hpp $(INC)/Graphtypes.hpp $(INC)/general.hpp $(INC)/TimeStamp.hpp
	$(CCPP) $(CPPFLAGS) -c Logtypes.cpp -o $(OBJ)/Logtypes.o

$(OBJ)/fzpostgres.o: fzpostgres.cpp $(INC)/fzpostgres.hpp $(INC)/error.hpp \
						$(INC)/general.hpp $(INC)/TimeStamp.hpp
	$(CCPP) $(CPPFLAGS) $(DEFAULTDBNAME) $(DEFAULTSCHEMA) -c fzpostgres.cpp -o $(OBJ)/fzpostgres.o

$(OBJ)/Graphaccess.o: Graphaccess.cpp $(INC)/Graphaccess.hpp $(INC)/standard.hpp $(INC)/fzpostgres.hpp
	$(CCPP) $(CPPFLAGS) $(DEFAULTDBNAME) $(DEFAULTSCHEMA) -c Graphaccess.cpp -o $(OBJ)/Graphaccess.o

$(OBJ)/Logaccess.o: Logaccess.cpp $(INC)/Logaccess.hpp $(INC)/standard.hpp $(INC)/Logpostgres.hpp $(INC)/fzpostgres.hpp
	$(CCPP) $(CPPFLAGS) $(DEFAULTDBNAME) $(DEFAULTSCHEMA) -c Logaccess.cpp -o $(OBJ)/Logaccess.o

$(OBJ)/Graphpostgres.o: Graphpostgres.cpp $(INC)/Graphpostgres.hpp $(INC)/Graphtypes.hpp \
						$(INC)/fzpostgres.hpp $(INC)/general.hpp $(INC)/error.hpp
	$(CCPP) $(CPPFLAGS) $(DEFAULTDBNAME) $(DEFAULTSCHEMA) -c Graphpostgres.cpp -o $(OBJ)/Graphpostgres.o

$(OBJ)/Logpostgres.o: Logpostgres.cpp $(INC)/Logpostgres.hpp $(INC)/Logtypes.hpp \
						$(INC)/fzpostgres.hpp $(INC)/general.hpp $(INC)/error.hpp
	$(CCPP) $(CPPFLAGS) $(DEFAULTDBNAME) $(DEFAULTSCHEMA) -c Logpostgres.cpp -o $(OBJ)/Logpostgres.o

$(OBJ)/Guidepostgres.o: Guidepostgres.cpp $(INC)/Guidepostgres.hpp $(INC)/fzpostgres.hpp \
						$(INC)/general.hpp $(INC)/error.hpp
	$(CCPP) $(CPPFLAGS) $(DEFAULTDBNAME) $(DEFAULTSCHEMA) -c Guidepostgres.cpp -o $(OBJ)/Guidepostgres.o

clean:
	rm -f $(OBJ)/*.o $(TEST)/*.o

#doc++:
#	rm -r -f html
#	doc++ -d html spiker.dxx
