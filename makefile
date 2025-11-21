# CPLEX_VERSION = 2211

# detecta se o sistema é de 32 ou 64 bits
BITS_OPTION = -m64

####diretorios com as libs do cplex
####diretorios com as libs do cplex

ifeq ($(shell uname),Darwin) # if OS X

	CPLEXDIR  = ~/Applications/IBM/ILOG/CPLEX_Studio1210/cplex
	CONCERTDIR = ~/Applications/IBM/ILOG/CPLEX_Studio1210/concert
	
	CPLEXLIBDIR   = $(CPLEXDIR)/lib/x86-64_osx/static_pic
	CONCERTLIBDIR = $(CONCERTDIR)/lib/x86-64_osx/static_pic

else # other unix

	CPLEXDIR  = /opt/ibm/ILOG/CPLEX_Studio1210/cplex
	CONCERTDIR = /opt/ibm/ILOG/CPLEX_Studio1210/concert
	
	CPLEXLIBDIR   = $(CPLEXDIR)/lib/x86-64_linux/static_pic
	CONCERTLIBDIR = $(CONCERTDIR)/lib/x86-64_linux/static_pic

endif

# CPLEXDIR  = /Applications/CPLEX_Studio$(CPLEX_VERSION)/cplex
# CONCERTDIR = /Applications/CPLEX_Studio$(CPLEX_VERSION)/concert
   
# CPLEXLIBDIR   = $(CPLEXDIR)/lib/arm64_osx/static_pic
# CONCERTLIBDIR = $(CONCERTDIR)/lib/arm64_osx/static_pic

#### define o compilador
CPPC = g++ 
#############################

#### diretorio OR-Tools
ORTOOLSDIR = $(HOME)/or-tools
ORTOOLS_BUILD = $(ORTOOLSDIR)/build
ORTOOLS_DEPS = $(ORTOOLS_BUILD)/_deps
# Diretório local para bibliotecas (portabilidade)
LOCAL_LIBDIR = lib
#############################

#### opcoes de compilacao e includes
CCOPT = $(BITS_OPTION) -O3 -fPIC -fexceptions -DNDEBUG -DIL_STD -std=c++17
CONCERTINCDIR = $(CONCERTDIR)/include
CPLEXINCDIR   = $(CPLEXDIR)/include
# Flags base para todos os arquivos
CCFLAGS_BASE = $(CCOPT) -fopenmp -I$(CPLEXINCDIR) -I$(CONCERTINCDIR) -Iinclude
# Flags adicionais para arquivos que usam OR-Tools
CCFLAGS_ORTOOLS = -I$(ORTOOLSDIR) -I$(ORTOOLS_BUILD) -I$(ORTOOLS_DEPS)/absl-src -I$(ORTOOLS_DEPS)/protobuf-src/src -I$(ORTOOLS_DEPS)/protobuf-src -I$(ORTOOLS_DEPS)/re2-src -I$(ORTOOLS_DEPS)/eigen3-src -DOR_PROTO_DLL=
# Flags padrão (sem OR-Tools)
CCFLAGS = $(CCFLAGS_BASE)
#############################

#### flags do linker
CCLNFLAGS = -L$(CPLEXLIBDIR) -lilocplex -lcplex -L$(CONCERTLIBDIR) -lconcert -lm -lpthread -ldl -fopenmp
# Flags de linkagem para OR-Tools
# Usa diretório local se existir, senão usa o diretório original do OR-Tools
ORTOOLS_LIB = $(ORTOOLS_BUILD)/lib
LOCAL_ORTOOLS_LIB = $(LOCAL_LIBDIR)
# Verifica se o diretório local existe, senão usa o original
ifeq ($(wildcard $(LOCAL_ORTOOLS_LIB)/libortools.so*),)
    # Se não existe lib local, usa a original
    CCLNFLAGS_ORTOOLS = -L$(ORTOOLS_LIB) -Wl,-rpath,$(ORTOOLS_LIB) -Wl,--no-as-needed -lortools -labsl_flags_parse -labsl_flags_usage -labsl_log_initialize -labsl_log_internal_message -labsl_log_globals -labsl_time -labsl_strings -labsl_base -lprotobuf -Wl,--as-needed
else
    # Usa bibliotecas locais com rpath relativo (portabilidade)
    # $$ORIGIN será expandido pelo linker para o diretório do executável
    CCLNFLAGS_ORTOOLS = -L$(LOCAL_ORTOOLS_LIB) -Wl,-rpath,$$ORIGIN/lib -Wl,--no-as-needed -lortools -labsl_flags_parse -labsl_flags_usage -labsl_log_initialize -labsl_log_internal_message -labsl_log_globals -labsl_time -labsl_strings -labsl_base -lprotobuf -Wl,--as-needed
endif
#############################

#### diretorios com os source files e com os objs files
SRCDIR = src
OBJDIR = obj
#############################

#### lista de todos os srcs e todos os objs (exclui Model-Highs.cpp que usa OR-Tools)
#### Model-OR-Tools.cpp, main.cpp, Combinations.cpp, Enumeration.cpp e Heuristic.cpp são compilados separadamente com regra especial abaixo
#### porque incluem headers do OR-Tools
SRCS = $(filter-out $(SRCDIR)/Model-Highs.cpp $(SRCDIR)/Model-OR-Tools.cpp $(SRCDIR)/main.cpp $(SRCDIR)/Combinations.cpp $(SRCDIR)/Enumeration.cpp $(SRCDIR)/Heuristic.cpp, $(wildcard $(SRCDIR)/*.cpp))
OBJS = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRCS))
OBJS += $(OBJDIR)/Model-OR-Tools.o $(OBJDIR)/main.o $(OBJDIR)/Combinations.o $(OBJDIR)/Enumeration.o $(OBJDIR)/Heuristic.o
#############################

#### regra principal, gera o executavel
cbtu: $(OBJS) 
	@echo  "\033[31m \nLinking all objects files: \033[0m"
	$(CPPC) $(BITS_OPTION) $(OBJS) -o $@ $(CCLNFLAGS) $(CCLNFLAGS_ORTOOLS)
############################

# inclui os arquivos de dependencias
-include $(OBJS:.o=.d)

# regra para cada arquivo objeto: compila e gera o arquivo de dependencias do arquivo objeto
# cada arquivo objeto depende do .c e dos headers (informacao dos header esta no arquivo de dependencias gerado pelo compiler)
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@echo  "\033[31m \nCompiling $<: \033[0m"
	$(CPPC) $(CCFLAGS) -c $< -o $@
	@echo  "\033[32m \ncreating $< dependency file: \033[0m"
	$(CPPC) -std=c++0x $(CCFLAGS) -MM $< > $(basename $@).d
	@mv -f $(basename $@).d $(basename $@).d.tmp #proximas tres linhas colocam o diretorio no arquivo de dependencias (g++ nao coloca, surprisingly!)
	@sed -e 's|.*:|$(basename $@).o:|' < $(basename $@).d.tmp > $(basename $@).d
	@rm -f $(basename $@).d.tmp

# Regra especial para Model-OR-Tools.cpp que requer OR-Tools
$(OBJDIR)/Model-OR-Tools.o: $(SRCDIR)/Model-OR-Tools.cpp
	@echo  "\033[31m \nCompiling $< with OR-Tools support: \033[0m"
	$(CPPC) $(CCFLAGS_BASE) $(CCFLAGS_ORTOOLS) -c $< -o $@
	@echo  "\033[32m \ncreating $< dependency file: \033[0m"
	$(CPPC) -std=c++0x $(CCFLAGS_BASE) $(CCFLAGS_ORTOOLS) -MM $< > $(basename $@).d
	@mv -f $(basename $@).d $(basename $@).d.tmp
	@sed -e 's|.*:|$(basename $@).o:|' < $(basename $@).d.tmp > $(basename $@).d
	@rm -f $(basename $@).d.tmp

# Regra especial para main.cpp que requer OR-Tools (porque inclui Model-OR-Tools.hpp)
$(OBJDIR)/main.o: $(SRCDIR)/main.cpp
	@echo  "\033[31m \nCompiling $< with OR-Tools support: \033[0m"
	$(CPPC) $(CCFLAGS_BASE) $(CCFLAGS_ORTOOLS) -c $< -o $@
	@echo  "\033[32m \ncreating $< dependency file: \033[0m"
	$(CPPC) -std=c++0x $(CCFLAGS_BASE) $(CCFLAGS_ORTOOLS) -MM $< > $(basename $@).d
	@mv -f $(basename $@).d $(basename $@).d.tmp
	@sed -e 's|.*:|$(basename $@).o:|' < $(basename $@).d.tmp > $(basename $@).d
	@rm -f $(basename $@).d.tmp

# Regra especial para Combinations.cpp que requer OR-Tools (porque Combinations.hpp inclui Model-OR-Tools.hpp)
$(OBJDIR)/Combinations.o: $(SRCDIR)/Combinations.cpp
	@echo  "\033[31m \nCompiling $< with OR-Tools support: \033[0m"
	$(CPPC) $(CCFLAGS_BASE) $(CCFLAGS_ORTOOLS) -c $< -o $@
	@echo  "\033[32m \ncreating $< dependency file: \033[0m"
	$(CPPC) -std=c++0x $(CCFLAGS_BASE) $(CCFLAGS_ORTOOLS) -MM $< > $(basename $@).d
	@mv -f $(basename $@).d $(basename $@).d.tmp
	@sed -e 's|.*:|$(basename $@).o:|' < $(basename $@).d.tmp > $(basename $@).d
	@rm -f $(basename $@).d.tmp

# Regra especial para Enumeration.cpp que requer OR-Tools (porque Enumeration.hpp inclui Model-OR-Tools.hpp)
$(OBJDIR)/Enumeration.o: $(SRCDIR)/Enumeration.cpp
	@echo  "\033[31m \nCompiling $< with OR-Tools support: \033[0m"
	$(CPPC) $(CCFLAGS_BASE) $(CCFLAGS_ORTOOLS) -c $< -o $@
	@echo  "\033[32m \ncreating $< dependency file: \033[0m"
	$(CPPC) -std=c++0x $(CCFLAGS_BASE) $(CCFLAGS_ORTOOLS) -MM $< > $(basename $@).d
	@mv -f $(basename $@).d $(basename $@).d.tmp
	@sed -e 's|.*:|$(basename $@).o:|' < $(basename $@).d.tmp > $(basename $@).d
	@rm -f $(basename $@).d.tmp

# Regra especial para Heuristic.cpp que requer OR-Tools (porque Heuristic.hpp inclui Model-OR-Tools.hpp)
$(OBJDIR)/Heuristic.o: $(SRCDIR)/Heuristic.cpp
	@echo  "\033[31m \nCompiling $< with OR-Tools support: \033[0m"
	$(CPPC) $(CCFLAGS_BASE) $(CCFLAGS_ORTOOLS) -c $< -o $@
	@echo  "\033[32m \ncreating $< dependency file: \033[0m"
	$(CPPC) -std=c++0x $(CCFLAGS_BASE) $(CCFLAGS_ORTOOLS) -MM $< > $(basename $@).d
	@mv -f $(basename $@).d $(basename $@).d.tmp
	@sed -e 's|.*:|$(basename $@).o:|' < $(basename $@).d.tmp > $(basename $@).d
	@rm -f $(basename $@).d.tmp

# delete objetos e arquivos de dependencia
clean:
	@echo "\033[31mcleaning obj directory \033[0m"
	@rm -f cbtu $(OBJDIR)/*.o $(OBJDIR)/*.d


rebuild: clean cbtu

# Target para copiar bibliotecas OR-Tools para diretório local (portabilidade)
# Isso permite executar o programa em outra máquina sem instalar OR-Tools
copy-ortools-libs:
	@echo "\033[31mCopiando bibliotecas OR-Tools para diretório local...\033[0m"
	@mkdir -p $(LOCAL_LIBDIR)
	@echo "Copiando bibliotecas principais..."
	@cp -L $(ORTOOLS_LIB)/libortools.so* $(LOCAL_LIBDIR)/ 2>/dev/null || true
	@cp -L $(ORTOOLS_LIB)/libprotobuf.so* $(LOCAL_LIBDIR)/ 2>/dev/null || true
	@echo "Copiando bibliotecas absl..."
	@cp -L $(ORTOOLS_LIB)/libabsl_*.so $(LOCAL_LIBDIR)/ 2>/dev/null || true
	@echo "Copiando outras dependências..."
	@cp -L $(ORTOOLS_LIB)/libre2.so* $(LOCAL_LIBDIR)/ 2>/dev/null || true
	@cp -L $(ORTOOLS_LIB)/libutf8_*.so* $(LOCAL_LIBDIR)/ 2>/dev/null || true
	@cp -L $(ORTOOLS_LIB)/libz.so* $(LOCAL_LIBDIR)/ 2>/dev/null || true
	@cp -L $(ORTOOLS_LIB)/libbz2.so* $(LOCAL_LIBDIR)/ 2>/dev/null || true
	@echo "\033[32mBibliotecas copiadas para $(LOCAL_LIBDIR)/\033[0m"
	@echo "\033[33mNota: Para usar em outra máquina, copie o diretório $(LOCAL_LIBDIR)/ junto com o executável\033[0m"

# Target para limpar bibliotecas locais
clean-libs:
	@echo "\033[31mRemovendo bibliotecas locais...\033[0m"
	@rm -rf $(LOCAL_LIBDIR)

# Target para compilar Model-Highs.cpp com OR-Tools (requer OR-Tools instalado)
# Nota: OR-Tools tem muitas dependências. Para compilar manualmente, use:
# g++ src/Model-Highs.cpp -o highs -I$(HOME)/or-tools -I$(HOME)/or-tools/build \
#     -I$(HOME)/or-tools/build/_deps/absl-src -I$(HOME)/or-tools/build/_deps/protobuf-src/src \
#     -L$(HOME)/or-tools/build/lib -lortools -lhighs -lpthread -std=c++17
highs:
	@echo "\033[31mCompilando Model-Highs.cpp com OR-Tools...\033[0m"
	$(CPPC) $(SRCDIR)/Model-Highs.cpp -o highs \
		-I$(HOME)/or-tools \
		-I$(HOME)/or-tools/build \
		-I$(HOME)/or-tools/build/_deps/absl-src \
		-I$(HOME)/or-tools/build/_deps/protobuf-src/src \
		-L$(HOME)/or-tools/build/lib \
		-lortools -lhighs -lpthread -std=c++17 \
		|| echo "\033[33mErro: Pode ser necessário ajustar os caminhos ou instalar dependências adicionais do OR-Tools\033[0m"

