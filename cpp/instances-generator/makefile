# Makefile para o random-generator
# Uso: make (compila o random-generator)
#      make clean (remove arquivos objeto e executável)
#      make COMPAT=1 (compila com static linking para melhor compatibilidade)
#      make GCC_VER=g++-11 (usa versão específica do GCC, ex: g++-11, g++-12)

# Configurações do CPLEX
CPLEXDIR = /opt/ibm/ILOG/CPLEX_Studio1210/cplex
CONCERTDIR = /opt/ibm/ILOG/CPLEX_Studio1210/concert

# Diretórios das bibliotecas
CPLEXLIBDIR = $(CPLEXDIR)/lib/x86-64_linux/static_pic
CONCERTLIBDIR = $(CONCERTDIR)/lib/x86-64_linux/static_pic

# Compilador - permite especificar versão específica do GCC
ifdef GCC_VER
    CPPC = $(GCC_VER)
else
    CPPC = g++
endif

# Opções de compilação
CCOPT = -m64 -O3 -fPIC -fexceptions -DNDEBUG -DIL_STD -std=c++17
CCFLAGS = $(CCOPT) -fopenmp -I$(CPLEXDIR)/include -I$(CONCERTDIR)/include -I../include

# Flags do linker
# Se COMPAT=1, usa static linking para libstdc++ e libgcc (melhora compatibilidade)
ifeq ($(COMPAT),1)
    CCLNFLAGS = -L$(CPLEXLIBDIR) -lilocplex -lcplex -L$(CONCERTLIBDIR) -lconcert -static-libgcc -static-libstdc++ -lm -lpthread -ldl -fopenmp
else
    CCLNFLAGS = -L$(CPLEXLIBDIR) -lilocplex -lcplex -L$(CONCERTLIBDIR) -lconcert -lm -lpthread -ldl -fopenmp
endif

# Diretórios
SRCDIR = ../src
OBJDIR = ../obj

# Arquivos fonte
RANDOM_GENERATOR_SRC = random-generator.cpp
DATA_SRC = $(SRCDIR)/Data.cpp
MODEL_SRC = $(SRCDIR)/Model.cpp
COMB_SRC = $(SRCDIR)/Combinations.cpp

# Arquivos objeto
RANDOM_GENERATOR_OBJ = $(OBJDIR)/random-generator.o
DATA_OBJ = $(OBJDIR)/Data.o
MODEL_OBJ = $(OBJDIR)/Model.o
COMB_OBJ = $(OBJDIR)/Combinations.o
# Executável
TARGET = random-generator

# Regra principal
$(TARGET): $(RANDOM_GENERATOR_OBJ) $(DATA_OBJ) $(MODEL_OBJ) $(COMB_OBJ)
	@echo "Linking $(TARGET)..."
	$(CPPC) -m64 $(RANDOM_GENERATOR_OBJ) $(DATA_OBJ) $(MODEL_OBJ) $(COMB_OBJ) -o $@ $(CCLNFLAGS)
	@echo "Executável $(TARGET) criado com sucesso!"

# Compilar random-generator.cpp
$(RANDOM_GENERATOR_OBJ): $(RANDOM_GENERATOR_SRC)
	@echo "Compilando $(RANDOM_GENERATOR_SRC)..."
	$(CPPC) $(CCFLAGS) -c $< -o $@

# Compilar Data.cpp
$(DATA_OBJ): $(DATA_SRC)
	@echo "Compilando $(DATA_SRC)..."
	$(CPPC) $(CCFLAGS) -c $< -o $@

# Compilar Combinations.cpp
$(COMB_OBJ): $(COMB_SRC)
	@echo "Compilando $(COMB_SRC)..."
	$(CPPC) $(CCFLAGS) -c $< -o $@

# Compilar Model.cpp
$(MODEL_OBJ): $(MODEL_SRC)
	@echo "Compilando $(MODEL_SRC)..."
	$(CPPC) $(CCFLAGS) -c $< -o $@

# Limpar arquivos gerados
clean:
	@echo "Limpando arquivos..."
	@rm -f $(RANDOM_GENERATOR_OBJ) $(DATA_OBJ) $(MODEL_OBJ) $(COMB_OBJ) $(TARGET)
	@echo "Limpeza concluída!"

# Recompilar tudo
rebuild: clean $(TARGET)

# Regra para executar o programa (após compilar)
run: $(TARGET)
	@echo "Executando $(TARGET)..."
	./$(TARGET)

# Ajuda
help:
	@echo "Comandos disponíveis:"
	@echo "  make              - Compila o random-generator"
	@echo "  make clean        - Remove arquivos objeto e executável"
	@echo "  make rebuild      - Recompila tudo do zero"
	@echo "  make run          - Compila e executa o programa"
	@echo "  make COMPAT=1     - Compila com static linking (melhor compatibilidade)"
	@echo "  make GCC_VER=g++-11 - Usa versão específica do GCC (ex: g++-11, g++-12)"
	@echo "  make help         - Mostra esta ajuda"
	@echo ""
	@echo "Nota sobre compatibilidade GLIBC:"
	@echo "  Se o executável não funcionar em outra máquina devido a versão do GLIBC,"
	@echo "  compile na máquina de destino ou use uma versão mais antiga do GCC:"
	@echo "    make GCC_VER=g++-11 COMPAT=1"

.PHONY: clean rebuild run help
