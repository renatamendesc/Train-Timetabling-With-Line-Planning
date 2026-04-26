# makefile for generator
# usage: make (builds generator)
#        make clean (removes object files and executable)
#        make COMPAT=1 (builds with static linking for better compatibility)
#        make GCC_VER=g++-11 (uses a specific GCC version, e.g. g++-11, g++-12)

ifdef CPLEX_VER
    CPLEX_BASE = /opt/ibm/ILOG/CPLEX_Studio$(CPLEX_VER)
else
    # default version — adjust for your main machine
    CPLEX_BASE = /opt/ibm/ILOG/CPLEX_Studio1210
endif

CPLEXDIR = $(CPLEX_BASE)/cplex
CONCERTDIR = $(CPLEX_BASE)/concert

# library directories
CPLEXLIBDIR = $(CPLEXDIR)/lib/x86-64_linux/static_pic
CONCERTLIBDIR = $(CONCERTDIR)/lib/x86-64_linux/static_pic

# compiler — allows specifying a particular GCC version
ifdef GCC_VER
    CPPC = $(GCC_VER)
else
    CPPC = g++
endif

# compilation options
CCOPT = -m64 -O3 -fPIC -fexceptions -DNDEBUG -DIL_STD -std=c++17
# project headers live in ./include (not repo-root ../include when building here)
CCFLAGS = $(CCOPT) -fopenmp -I$(CPLEXDIR)/include -I$(CONCERTDIR)/include -Iinclude

# linker flags
# If COMPAT=1, link libstdc++ and libgcc statically (improves compatibility)
ifeq ($(COMPAT),1)
    CCLNFLAGS = -L$(CPLEXLIBDIR) -lilocplex -lcplex -L$(CONCERTLIBDIR) -lconcert -static-libgcc -static-libstdc++ -lm -lpthread -ldl -fopenmp
else
    CCLNFLAGS = -L$(CPLEXLIBDIR) -lilocplex -lcplex -L$(CONCERTLIBDIR) -lconcert -lm -lpthread -ldl -fopenmp
endif

# directories
SRCDIR = src
OBJDIR = obj

# source files
GENERATOR_SRC = generator.cpp
DATA_SRC = $(SRCDIR)/Data.cpp
MODEL_SRC = $(SRCDIR)/Model.cpp

# object files
GENERATOR_OBJ = $(OBJDIR)/generator.o
DATA_OBJ = $(OBJDIR)/Data.o
MODEL_OBJ = $(OBJDIR)/Model.o
# executable
TARGET = generator

# default target
$(TARGET): $(GENERATOR_OBJ) $(DATA_OBJ) $(MODEL_OBJ)
	@echo "Linking $(TARGET)..."
	$(CPPC) -m64 $(GENERATOR_OBJ) $(DATA_OBJ) $(MODEL_OBJ) -o $@ $(CCLNFLAGS)
	@echo "Executable $(TARGET) built successfully."

# compile generator.cpp
$(GENERATOR_OBJ): $(GENERATOR_SRC)
	@echo "Compiling $(GENERATOR_SRC)..."
	$(CPPC) $(CCFLAGS) -c $< -o $@

# compile Data.cpp
$(DATA_OBJ): $(DATA_SRC)
	@echo "Compiling $(DATA_SRC)..."
	$(CPPC) $(CCFLAGS) -c $< -o $@

# compile Model.cpp
$(MODEL_OBJ): $(MODEL_SRC)
	@echo "Compiling $(MODEL_SRC)..."
	$(CPPC) $(CCFLAGS) -c $< -o $@

# remove generated files
clean:
	@echo "Cleaning..."
	@rm -f $(GENERATOR_OBJ) $(DATA_OBJ) $(MODEL_OBJ) $(TARGET)
	@echo "Clean complete."

# rebuild from scratch
rebuild: clean $(TARGET)

# run the program (after building)
run: $(TARGET)
	@echo "Running $(TARGET)..."
	./$(TARGET)

# help
help:
	@echo "Available commands:"
	@echo "  make              - Build generator"
	@echo "  make clean        - Remove object files and executable"
	@echo "  make rebuild      - Full clean rebuild"
	@echo "  make run          - Build and run the program"
	@echo "  make COMPAT=1     - Build with static linking (better compatibility)"
	@echo "  make GCC_VER=g++-11 - Use a specific GCC (e.g. g++-11, g++-12)"
	@echo "  make help         - Show this help"
	@echo ""
	@echo "GLIBC compatibility note:"
	@echo "  If the binary fails on another machine due to GLIBC version,"
	@echo "  build on the target machine or use an older GCC:"
	@echo "    make GCC_VER=g++-11 COMPAT=1"

.PHONY: clean rebuild run help
