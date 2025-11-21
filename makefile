# Makefile para projeto Train-Timetabling com OR-Tools e HiGHS
# Gera executável portável que pode ser executado em máquinas sem OR-Tools instalado

# Compilador
CPPC = g++

# Diretórios OR-Tools (instalação padrão do GitHub)
ORTOOLS_INCLUDE = /usr/local/include
ORTOOLS_LIB = /usr/local/lib

# Diretório local para bibliotecas (portabilidade)
LOCAL_LIBDIR = lib

# Diretórios do projeto
SRCDIR = src
OBJDIR = obj
INCLUDEDIR = include

# Flags de compilação
CCFLAGS = -m64 -O3 -fPIC -fexceptions -DNDEBUG -std=c++17 \
          -fopenmp \
          -I$(INCLUDEDIR) \
          -I$(ORTOOLS_INCLUDE) \
          -I$(ORTOOLS_INCLUDE)/ortools \
          -DOR_PROTO_DLL=

# Flags de linkagem
# Usa biblioteca estática do OR-Tools e bibliotecas dinâmicas das dependências
# O rpath aponta para o diretório lib/ relativo ao executável (portabilidade)
# Inclui todas as bibliotecas Abseil necessárias (o linker remove as não usadas)
# Permite símbolos não definidos do SCIP (não usado, apenas HiGHS)
CCLNFLAGS = -L$(ORTOOLS_LIB) \
            -Wl,-rpath,'$$ORIGIN/$(LOCAL_LIBDIR)' \
            -Wl,-rpath,$(ORTOOLS_LIB) \
            -Wl,--unresolved-symbols=ignore-in-object-files \
            -lortools \
            -labsl_flags_parse -labsl_flags_usage -labsl_log_initialize \
            -labsl_log_internal_message -labsl_log_globals -labsl_log_internal_nullguard \
            -labsl_time -labsl_int128 -labsl_leak_check \
            -labsl_strings -labsl_strings_internal -labsl_string_view -labsl_base -labsl_raw_logging_internal \
            -labsl_str_format_internal -labsl_statusor -labsl_status \
            -labsl_cord -labsl_cord_internal -labsl_cordz_functions \
            -labsl_cordz_handle -labsl_cordz_info -labsl_cordz_sample_token \
            -labsl_hash -labsl_city -labsl_low_level_hash -labsl_raw_hash_set \
            -labsl_random_distributions \
            -labsl_random_internal_platform -labsl_random_internal_randen \
            -labsl_random_internal_randen_hwaes -labsl_random_internal_randen_hwaes_impl -labsl_random_internal_randen_slow \
            -labsl_random_seed_sequences -labsl_random_internal_entropy_pool -labsl_random_internal_seed_material \
            -labsl_synchronization -labsl_graphcycles_internal -labsl_spinlock_wait -labsl_kernel_timeout_internal \
            -labsl_symbolize -labsl_examine_stack -labsl_failure_signal_handler \
            -labsl_debugging_internal -labsl_demangle_internal -labsl_tracing_internal \
            -labsl_stacktrace -labsl_throw_delegate -labsl_civil_time -labsl_time_zone \
            -labsl_strerror -labsl_periodic_sampler -labsl_exponential_biased \
            -labsl_hashtablez_sampler -labsl_scoped_set_env -labsl_poison \
            -labsl_die_if_null -labsl_utf8_for_code_point -labsl_decode_rust_punycode \
            -labsl_demangle_rust -labsl_flags_commandlineflag -labsl_flags_commandlineflag_internal \
            -labsl_flags_config -labsl_flags_internal -labsl_flags_marshalling \
            -labsl_flags_private_handle_accessor -labsl_flags_program_name \
            -labsl_flags_reflection -labsl_flags_usage_internal -labsl_log_flags \
            -labsl_log_internal_check_op -labsl_log_internal_conditions \
            -labsl_log_internal_fnmatch -labsl_log_internal_format \
            -labsl_log_internal_log_sink_set -labsl_log_internal_proto \
            -labsl_log_internal_structured_proto -labsl_log_severity -labsl_log_sink \
            -labsl_vlog_config_internal             -labsl_random_internal_distribution_test_util \
            -labsl_random_seed_gen_exception \
            -labsl_crc32c -labsl_crc_cpu_detect -labsl_crc_cord_state \
            -labsl_crc_internal -labsl_malloc_internal \
            -lprotobuf -lre2 -lhighs -lz \
            -lm -lpthread -ldl -fopenmp

# Lista de sources e objs (exclui Model.cpp que depende do CPLEX)
SRCS = $(filter-out $(SRCDIR)/Model.cpp,$(wildcard $(SRCDIR)/*.cpp))
OBJS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

# Executável portável
cbtu: copy-libs $(OBJS)
	@echo "\033[31m\nLinking all object files (portable version):\033[0m"
	$(CPPC) -m64 $(OBJS) -o $@ $(CCLNFLAGS)
	@echo "\033[32m\nExecutável portável criado: cbtu\033[0m"
	@echo "\033[33mDistribua junto: cbtu e diretório lib/\033[0m"

# Compilação de objetos
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	@echo "\033[31m\nCompiling $<:\033[0m"
	$(CPPC) $(CCFLAGS) -c $< -o $@

# Copiar bibliotecas necessárias para diretório local (portabilidade)
copy-libs:
	@echo "\033[31mCopying required libraries locally...\033[0m"
	@mkdir -p $(LOCAL_LIBDIR)
	@echo "  Copying HiGHS library..."
	@cp -L $(ORTOOLS_LIB)/libhighs.so* $(LOCAL_LIBDIR)/ 2>/dev/null || true
	@echo "  Copying Abseil libraries..."
	@cp -L $(ORTOOLS_LIB)/libabsl*.so* $(LOCAL_LIBDIR)/ 2>/dev/null || true
	@echo "  Copying Protobuf library..."
	@if ls /lib/x86_64-linux-gnu/libprotobuf.so* 1> /dev/null 2>&1; then \
		cp -L /lib/x86_64-linux-gnu/libprotobuf.so* $(LOCAL_LIBDIR)/ 2>/dev/null || true; \
	elif ls $(ORTOOLS_LIB)/libprotobuf.so* 1> /dev/null 2>&1; then \
		cp -L $(ORTOOLS_LIB)/libprotobuf.so* $(LOCAL_LIBDIR)/ 2>/dev/null || true; \
	fi
	@echo "  Copying RE2 library..."
	@if ls /lib/x86_64-linux-gnu/libre2.so* 1> /dev/null 2>&1; then \
		cp -L /lib/x86_64-linux-gnu/libre2.so* $(LOCAL_LIBDIR)/ 2>/dev/null || true; \
	elif ls $(ORTOOLS_LIB)/libre2.so* 1> /dev/null 2>&1; then \
		cp -L $(ORTOOLS_LIB)/libre2.so* $(LOCAL_LIBDIR)/ 2>/dev/null || true; \
	fi
	@echo "\033[32mAll required libraries copied to $(LOCAL_LIBDIR)/\033[0m"

# Limpeza
clean:
	@echo "\033[31mCleaning obj directory and local libraries\033[0m"
	@rm -rf $(OBJDIR)/*.o $(OBJDIR)/*.d cbtu $(LOCAL_LIBDIR)

.PHONY: copy-libs clean
