# Makefile com OR-Tools e HiGHS

# Compilador
CXX = g++
CXXFLAGS = -std=c++17 -O2 -pthread -fopenmp

# Diretório base do OR-Tools
# Primeiro tenta pegar da variável de ambiente ORTOOLS_DIR
# Senão, tenta caminhos padrão locais e no servidor
ORTOOLS ?= $(HOME)/or-tools/or-tools_x86_64_Ubuntu-24.04_cpp_v9.14.6206
ORTOOLS_EXISTS := $(wildcard $(ORTOOLS)/lib/libortools.so)
ifeq ($(ORTOOLS_EXISTS),)
    ORTOOLS := /opt/or-tools
endif

# Diretórios do projeto
SRCDIR = src
OBJDIR = obj
INCLUDEDIR = include

# Flags de compilação
INCLUDES = -I$(INCLUDEDIR) -I$(ORTOOLS)/include

# Flags de linkagem
LIBS = -Wl,-rpath,$(ORTOOLS)/lib \
       -L$(ORTOOLS)/lib \
       -lortools \
       -labsl_strings \
       -labsl_strings_internal \
       -labsl_base \
       -labsl_raw_logging_internal \
       -labsl_log_internal_message \
       -labsl_log_globals \
       -labsl_log_initialize \
       -labsl_log_internal_nullguard \
       -labsl_time \
       -labsl_int128 \
       -labsl_string_view \
       -labsl_statusor \
       -labsl_status \
       -labsl_cord \
       -labsl_cord_internal \
       -labsl_hash \
       -labsl_city \
       -labsl_low_level_hash \
       -labsl_raw_hash_set \
       -labsl_synchronization \
       -labsl_str_format_internal \
       -lprotobuf \
       -lre2 \
       -lpthread \
       -fopenmp

# Lista de sources e objs (exclui Model.cpp que depende do CPLEX)
SRCS = $(filter-out $(SRCDIR)/Model.cpp,$(wildcard $(SRCDIR)/*.cpp))
OBJS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

# Executável
cbtu: $(OBJS)
	@echo "\033[31m\nLinking all object files:\033[0m"
	$(CXX) $(CXXFLAGS) $(OBJS) $(LIBS) -o $@
	@echo "\033[32m\nExecutável criado: cbtu\033[0m"

# Compilação de objetos
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	@echo "\033[31m\nCompiling $<:\033[0m"
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Limpeza
clean:
	@echo "\033[31mCleaning obj directory and executable\033[0m"
	@rm -rf $(OBJDIR)/*.o $(OBJDIR)/*.d cbtu

.PHONY: clean
