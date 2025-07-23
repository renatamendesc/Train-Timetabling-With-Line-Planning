# —————————————————————————————————————————————————————————————
# 1) VERSÃO CPLEX e DIRETÓRIOS
# —————————————————————————————————————————————————————————————
CPLEX_VERSION = 2212
BASE_DIR     = /opt/ibm/ILOG/CPLEX_Studio$(CPLEX_VERSION)

ifeq ($(shell uname),Darwin)
  CPLEXLIBDIR   = $(BASE_DIR)/cplex/lib/x86-64_osx/static_pic
  CONCERTLIBDIR = $(BASE_DIR)/concert/lib/x86-64_osx/static_pic
else
  CPLEXLIBDIR   = $(BASE_DIR)/cplex/lib/x86-64_linux/static_pic
  CONCERTLIBDIR = $(BASE_DIR)/concert/lib/x86-64_linux/static_pic
endif

CONCERTINCDIR = $(BASE_DIR)/concert/include
CPLEXINCDIR   = $(BASE_DIR)/cplex/include

# —————————————————————————————————————————————————————————————
# 2) COMPILADORES E FLAGS
# —————————————————————————————————————————————————————————————
# Compiladores
HOSTCC = nvcc
NVCC   = nvcc
LD     = nvcc  # <--- alterado de g++ para nvcc

# Flags
HOSTFLAGS = -m64 -O3 -Xcompiler="-fPIC -fexceptions -DNDEBUG -DIL_STD" \
            -std=c++17 -Xcompiler -fopenmp \
            -I$(CPLEXINCDIR) -I$(CONCERTINCDIR) -Iinclude

NVCCFLAGS = -m64 -O3 -std=c++17 \
            -Xcompiler="-fPIC -fexceptions -DNDEBUG" \
            -I$(CPLEXINCDIR) -I$(CONCERTINCDIR) -Iinclude

LDFLAGS  = -m64 \
           -L$(CPLEXLIBDIR) -lilocplex -lcplex \
           -L$(CONCERTLIBDIR) -lconcert \
           -lm -lpthread -ldl -fopenmp

# —————————————————————————————————————————————————————————————
# 3) SRC/OBJ DIRETÓRIOS E EXTENSÕES
# —————————————————————————————————————————————————————————————
SRCDIR    = src
OBJDIR    = obj

# Removemos Combinations.cpp manualmente daqui
CPP_SRCS  = $(filter-out $(SRCDIR)/Combinations.cpp, $(wildcard $(SRCDIR)/*.cpp))
CU_SRCS   = $(wildcard $(SRCDIR)/*.cu)
CUH_HDRS  = $(wildcard $(SRCDIR)/*.cuh)

CPP_OBJS  = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(CPP_SRCS))
CU_OBJS   = $(patsubst $(SRCDIR)/%.cu, $(OBJDIR)/%.cu.o, $(CU_SRCS))
OBJS      = $(CPP_OBJS) $(CU_OBJS) $(OBJDIR)/Combinations.o

# —————————————————————————————————————————————————————————————
# 4) ALVO PRINCIPAL
# —————————————————————————————————————————————————————————————
cbtu: $(OBJS)
	@echo "\n\033[31mLinking all objects into $@ …\033[0m"
	$(LD) -o $@ $(OBJS) $(LDFLAGS)

# —————————————————————————————————————————————————————————————
# 5) REGRAS DE COMPILAÇÃO
# —————————————————————————————————————————————————————————————

# 5.1) Compilar Combinations.cpp com nvcc -x cu (reconhece <<<>>>)
$(OBJDIR)/Combinations.o: $(SRCDIR)/Combinations.cpp $(CUH_HDRS)
	@mkdir -p $(OBJDIR)
	@echo "Compiling CUDA host+kernel $< with nvcc…"
	$(NVCC) $(NVCCFLAGS) -x cu -c $< -o $@

# 5.2) Regra genérica para demais .cpp
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(CUH_HDRS)
	@mkdir -p $(OBJDIR)
	@echo "Compiling $< with $(HOSTCC)…"
	$(HOSTCC) $(HOSTFLAGS) -c $< -o $@

# 5.3) Regras para arquivos .cu (com extensão .cu.o)
$(OBJDIR)/%.cu.o: $(SRCDIR)/%.cu $(CUH_HDRS)
	@mkdir -p $(OBJDIR)
	@echo "Compiling CUDA kernel $< with nvcc…"
	$(NVCC) $(NVCCFLAGS) -c $< -o $@

# —————————————————————————————————————————————————————————————
# 6) LIMPEZA
# —————————————————————————————————————————————————————————————
clean:
	@echo "\n\033[31mCleaning $(OBJDIR) and binary\033[0m"
	@rm -f cbtu $(OBJDIR)/*.{o,cu.o,d}

rebuild: clean cbtu
