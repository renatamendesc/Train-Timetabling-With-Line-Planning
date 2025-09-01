# Makefile para o random-generator
# Uso: make (compila o random-generator)
#      make clean (remove arquivos objeto e executável)

# Configurações do CPLEX
CPLEX_VERSION = 2211
CPLEXDIR = /opt/ibm/ILOG/CPLEX_Studio201/cplex
CONCERTDIR = /opt/ibm/ILOG/CPLEX_Studio201/concert

# Diretórios das bibliotecas
CPLEXLIBDIR = $(CPLEXDIR)/lib/x86-64_linux/static_pic
CONCERTLIBDIR = $(CONCERTDIR)/lib/x86-64_linux/static_pic

# Compilador
CPPC = g++

# Opções de compilação
CCOPT = -m64 -O3 -fPIC -fexceptions -DNDEBUG -DIL_STD -std=c++17
CCFLAGS = $(CCOPT) -fopenmp -I$(CPLEXDIR)/include -I$(CONCERTDIR)/include -I../../include

# Flags do linker
CCLNFLAGS = -L$(CPLEXLIBDIR) -lilocplex -lcplex -L$(CONCERTLIBDIR) -lconcert -lm -lpthread -ldl -fopenmp

# Diretórios
SRCDIR = ../../src
OBJDIR = ../../obj

# Arquivos fonte
RANDOM_GENERATOR_SRC = random-generator.cpp
DATA_SRC = $(SRCDIR)/Data.cpp
MODEL_SRC = $(SRCDIR)/Model.cpp

# Arquivos objeto
RANDOM_GENERATOR_OBJ = $(OBJDIR)/random-generator.o
DATA_OBJ = $(OBJDIR)/Data.o
MODEL_OBJ = $(OBJDIR)/Model.o

# Executável
TARGET = random-generator

# Regra principal
$(TARGET): $(RANDOM_GENERATOR_OBJ) $(DATA_OBJ) $(MODEL_OBJ)
	@echo "Linking $(TARGET)..."
	$(CPPC) -m64 $(RANDOM_GENERATOR_OBJ) $(DATA_OBJ) $(MODEL_OBJ) -o $@ $(CCLNFLAGS)
	@echo "Executável $(TARGET) criado com sucesso!"

# Compilar random-generator.cpp
$(RANDOM_GENERATOR_OBJ): $(RANDOM_GENERATOR_SRC)
	@echo "Compilando $(RANDOM_GENERATOR_SRC)..."
	$(CPPC) $(CCFLAGS) -c $< -o $@

# Compilar Data.cpp
$(DATA_OBJ): $(DATA_SRC)
	@echo "Compilando $(DATA_SRC)..."
	$(CPPC) $(CCFLAGS) -c $< -o $@

# Compilar Model.cpp
$(MODEL_OBJ): $(MODEL_SRC)
	@echo "Compilando $(MODEL_SRC)..."
	$(CPPC) $(CCFLAGS) -c $< -o $@

# Limpar arquivos gerados
clean:
	@echo "Limpando arquivos..."
	@rm -f $(RANDOM_GENERATOR_OBJ) $(DATA_OBJ) $(MODEL_OBJ) $(TARGET)
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
	@echo "  make        - Compila o random-generator"
	@echo "  make clean  - Remove arquivos objeto e executável"
	@echo "  make rebuild- Recompila tudo do zero"
	@echo "  make run    - Compila e executa o programa"
	@echo "  make help   - Mostra esta ajuda"

.PHONY: clean rebuild run help
