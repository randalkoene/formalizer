#
# Makefile for dil2graph
# (by Randal A. Koene, randalk@minduploading.org)
#

SHELL=/usr/bin/bash

# +----- begin: Output preconfigurations -----+
STANDARD_STREAMS = -DFORMALIZER_BASE_OUT_OSTREAM_PTR=\&std::cout
STANDARD_STREAMS += -DFORMALIZER_BASE_ERR_OSTREAM_PTR=\&std::cerr
STANDARD_STREAMS += -DTESTSTREAMS
# +----- end  : Output preconfigurations -----+

##########################################################
## Compiler
## --------
CC=gcc
CCPP=g++ 
##########################################################

##########################################################
## Compiler Options
## ----------------
## debugging with vscode (update 2020-09-15):
#COMPOPT= -g -O0
#OPTSTR=debugging_with_vscode
## debugging information (updates 2019-02-01):
#COMPOPT= -g -Og
#OPTSTR=editing_optimized_for_debugging
## generate profile information for use with gprof:
#COMPOPT= -g -O6
#OPTSTR=profiling
## optimized (USUALLY USE THIS ONE!):
COMPOPT= -O3
OPTSTR=MAX_optimized_NO_debug_symbols
## optimized + debugging info (as per GDB recommendations 2019-02-01)
#COMPOPT= -g -O3
#OPTSTR=production_optimized_with_dbg_symbols
##########################################################

# Temporarily enable the following (later replace with graph2dil and server calls)
# COMPILEDIRECTGRAPH2DIL = -D__DIRECTGRAPH2DIL__ -DDIRECTGRAPH2DIL_DIR=\"/var/www/html/formalizer/tools/conversion/graph2dil\"
# OBJECTDIRECTGRAPH2DIL = ../graph2dil/obj/log2tl.o
# OBJECTDIRECTGRAPHDETECT = ../graph2dil/log2tl.cpp
 
# Define (uncomment) the following to set a different precompiled default
# output path for logtest.
LOGTESTOUTPUTFILE = -DDEFAULT_LOGTEST_FILE=\"/var/www/html/formalizer/dil2graph-testlogdata.html\"

FORMALIZERPATH=$(HOME)/src/formalizer
COREINCLUDEPATH=$(FORMALIZERPATH)/core/include
CORELIBPATH=$(FORMALIZERPATH)/core/lib
FORMALIZERPATH_THIS=$(FORMALIZERPATH)/tools/conversion/dil2graph

LOGTESTTEMPLATES = -DDEFAULT_TEMPLATE_DIR=\"$(FORMALIZERPATH_THIS)\"

DIL2AL=$(HOME)/src/dil2al

INC=$(COREINCLUDEPATH)
OBJ=./obj
TEST=./test

# to find the postgresql library files fo this: pg_config --libdir
LIB_PATH = -L/usr/lib/x86_64-linux-gnu -lpq

# To find the Boost libraries
# See the note on POSIX systems in Building Boost.Interprocess at https://www.boost.org/doc/libs/1_38_0/doc/html/interprocess.html#interprocess.intro.introduction_building_interprocess.
BOOSTINTERPROCESSLIB_REQS = -lrt -lpthread
LOCALINC = $(HOME)/local/include
LOCALLIB = $(BOOSTINTERPROCESSLIB_REQS)

# +----- begin: Essential include and library paths for this program -----+
# to find the postgresql header files (e.g libpq-fe.h), do this: pg_config --includedir
INCLUDES := -I$(CPATH) -I$(COREINCLUDEPATH)
INCLUDES += -I/usr/include/postgresql -I$(DIL2AL)
INCLUDES += -I$(LOCALINC)
LIB_PATH += $(LOCALLIB)
# +----- begin: Essential include and library paths for this program -----+

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
#RXSPECIAL=-Wno-error

##########################################################
## Regular Expression Library
## --------------------------
## (Suggestion: Using regex-gnu.h guarantees the greatest
## amount of compatibility and identical behaviour on all
## platforms, since it is included with the dil2al source
## code and has been adapted for reliable use with C++.)
##
## regex-gnu.h adapted for integration with C++:
GCC3FXTRA=$(if $(shell gcc -v 2>&1 | grep "^gcc version[^0-9]*[3-9][.]"),-Wno-unused-function)
ALT_REGEX=$(DIL2AL)/regex-gnu.o
ALT_REGEX_H=\"regex-gnu.h\"
CFXTRA= -D__DIL2AL__ -DSTDC_HEADERS -pedantic -Wall -Werror -Wno-char-subscripts $(GCC3FXTRA)
RXCFXTRA= -D__DIL2AL__ -DSTDC_HEADERS -pedantic -Wall $(GCC3FXTRA)
## dil2graph uses a few C++ constructs that are new to C++20, such as designated initializer lists,
## so let's add support for that here:
CPP2020= -std=gnu++2a
CPPXTRA= -D__DIL2AL__ -D_ALT_REGEX_H=$(ALT_REGEX_H) $(MACHOPT) -mieee-fp -ffast-math -D_USE_ALT_REGEX -D_CPP_REGEX -DDEFAULTHOMEDIR=\"$(HOME)\" -pedantic -Wall -Werror $(GCC3FXTRA) $(CPP2020)
REGEXSTR=regex-gnu_for_C++
##
##########################################################

##########################################################
## Safe Regular Expressions
## ------------------------
## assume '\0' within String length and rm structure has
## no sp/ep pointers:
#SAFEREGEX=
## make no assumptions:
SAFEREGEX= -D_BIGREGEX_SAFE_MATCHES
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
RXCFLAGS= $(COMPOPT) $(MACHSPEC) $(RXCFXTRA)
CPPFLAGS= $(COMPOPT) $(CPPOPT) $(MACHSPEC) $(CPPXTRA) $(SAFEREGEX) $(INCLUDES)

# +----- begin: Select Formalizer core library objects -----+
CORE_OBJS = $(CORELIBPATH)/obj/error.o
CORE_OBJS += $(CORELIBPATH)/obj/config.o
CORE_OBJS += $(CORELIBPATH)/obj/general.o
CORE_OBJS += $(CORELIBPATH)/obj/utf8.o
CORE_OBJS += $(CORELIBPATH)/obj/standard.o
CORE_OBJS += $(CORELIBPATH)/obj/templater.o
CORE_OBJS += $(CORELIBPATH)/obj/ReferenceTime.o
CORE_OBJS += $(CORELIBPATH)/obj/TimeStamp.o
CORE_OBJS += $(CORELIBPATH)/obj/Graphbase.o
CORE_OBJS += $(CORELIBPATH)/obj/Graphtypes.o
CORE_OBJS += $(CORELIBPATH)/obj/LogtypesID.o
CORE_OBJS += $(CORELIBPATH)/obj/Logtypes.o
CORE_OBJS += $(CORELIBPATH)/obj/fzpostgres.o
CORE_OBJS += $(CORELIBPATH)/obj/Graphaccess.o
CORE_OBJS += $(CORELIBPATH)/obj/Graphpostgres.o
CORE_OBJS += $(CORELIBPATH)/obj/Graphcompare.o
CORE_OBJS += $(CORELIBPATH)/obj/Logpostgres.o
CORE_OBJS += $(CORELIBPATH)/obj/dilaccess.o
# +----- end  : Select Formalizer core library objects -----+

DIL2AL_OBJS= $(DIL2AL)/utilities.o $(DIL2AL)/interface.o $(DIL2AL)/testcode.o $(DIL2AL)/diladmin.o \
			$(DIL2AL)/tladmin.o $(DIL2AL)/cksum.o $(DIL2AL)/alcomp.o $(DIL2AL)/tlfilter.o \
			$(DIL2AL)/BigString.o $(DIL2AL)/BigRegex.o

.PHONY: FORCE

all: init dil2graph

init: FORCE
	@echo '-------------------------------------------------------------------'
	@echo 'Compilation options: $(MACHSTR), $(OPTSTR), $(REGEXSTR)'
	@echo 'Includes: $(CPPFLAGS)'
	@echo '-------------------------------------------------------------------'

# add $(CPPTEMPWORKAROUND) as needed

$(OBJ)/dil2al_minimal.o: dil2al_minimal.cpp dil2al_minimal.hpp version.hpp $(INC)/standard.hpp $(INC)/error.hpp
	$(CCPP) $(CPPFLAGS) $(COLORS) $(STANDARD_STREAMS) -c dil2al_minimal.cpp -o $(OBJ)/dil2al_minimal.o

$(OBJ)/logtest.o: logtest.cpp logtest_templates.cpp logtest.hpp version.hpp $(INC)/standard.hpp $(INC)/error.hpp
	$(CCPP) $(CPPFLAGS) $(COLORS) $(STANDARD_STREAMS) $(LOGTESTTEMPLATES) $(LOGTESTOUTPUTFILE) -c logtest.cpp -o $(OBJ)/logtest.o

$(OBJ)/tl2log.o: tl2log.cpp tl2log.hpp version.hpp $(INC)/standard.hpp $(INC)/error.hpp
	$(CCPP) $(CPPFLAGS) $(COLORS) $(STANDARD_STREAMS) $(COMPILEDIRECTGRAPH2DIL) -c tl2log.cpp -o $(OBJ)/tl2log.o

$(OBJ)/dil2graph.o: dil2graph.cpp dil2graph.hpp version.hpp $(OBJ)/dil2al_minimal.o $(INC)/standard.hpp $(INC)/error.hpp
	$(CCPP) $(CPPFLAGS) $(COLORS) $(STANDARD_STREAMS) $(COMPILEDIRECTGRAPH2DIL) -c dil2graph.cpp -o $(OBJ)/dil2graph.o

$(OBJECTDIRECTGRAPH2DIL): $(OBJECTDIRECTGRAPHDETECT)
	@echo 'log2tl code has changed - please recompile in ../graph2dil !'

dil2graph: $(OBJ)/dil2graph.o $(OBJ)/tl2log.o $(OBJ)/logtest.o $(OBJ)/dil2al_minimal.o \
			$(DIL2AL_OBJS) $(CORE_OBJS) $(ALT_REGEX) $(OBJECTDIRECTGRAPH2DIL)
	$(CCPP) $(CPPFLAGS) $^ -o dil2graph $(LIB_PATH)

clean:
	cp -f dil2graph dil2graph.bak
	rm -f $(OBJ)/*.o dil2graph

#doc++:
#	rm -r -f html
#	doc++ -d html spiker.dxx

