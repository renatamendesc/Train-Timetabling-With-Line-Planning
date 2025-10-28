#!/bin/bash

# Script para executar uma única instância do CBTU e salvar o log
# Uso: ./run-single.sh <instancia.txt> <método> <threads>

EXECUTABLE="./cbtu"

# Função para mostrar ajuda
show_help() {
    echo "=== EXECUTOR DE INSTÂNCIA ÚNICA CBTU ==="
    echo ""
    echo "Uso: $0 <instancia.txt> <método> <threads>"
    echo ""
    echo "Parâmetros:"
    echo "  instancia.txt  - Arquivo da instância (.txt)"
    echo "  método         - model, enum ou heuristic"
    echo "  threads        - Número de threads (ex: 4)"
    echo ""
    echo "Exemplos:"
    echo "  $0 instances/experiments/5-to-9/t2p5r2h1.txt model 4"
    echo "  $0 instances/experiments/real/t3p16r10h1.txt heuristic 2"
    echo ""
    echo "O log será salvo em: solutions/timetables/<nome_da_instancia>.log"
}

# Verificar argumentos
if [[ $# -ne 3 ]]; then
    echo "Erro: Número incorreto de argumentos!"
    echo ""
    show_help
    exit 1
fi

INSTANCE_FILE="$1"
METHOD="$2"
THREADS="$3"

# Verificar se o arquivo da instância existe
if [[ ! -f "$INSTANCE_FILE" ]]; then
    echo "Erro: Arquivo da instância '$INSTANCE_FILE' não encontrado!"
    exit 1
fi

# Verificar se o executável existe
if [[ ! -f "$EXECUTABLE" ]]; then
    echo "Erro: Executável '$EXECUTABLE' não encontrado!"
    echo "Execute 'make' primeiro para compilar o programa."
    exit 1
fi

# Validar método
if [[ "$METHOD" != "model" && "$METHOD" != "enum" && "$METHOD" != "heuristic" ]]; then
    echo "Erro: Método inválido '$METHOD'"
    echo "Métodos válidos: model, enum, heuristic"
    exit 1
fi

# Validar número de threads
if ! [[ "$THREADS" =~ ^[0-9]+$ ]] || [[ "$THREADS" -lt 1 ]]; then
    echo "Erro: Número de threads inválido '$THREADS'"
    echo "Deve ser um número inteiro positivo."
    exit 1
fi

# Extrair nome da instância (sem extensão)
INSTANCE_NAME=$(basename "$INSTANCE_FILE" .txt)

# Criar pasta de destino se não existir
mkdir -p solutions/timetables

# Definir arquivo de log
LOG_FILE="solutions/timetables/${INSTANCE_NAME}.log"

echo "=== EXECUTANDO INSTÂNCIA ÚNICA ==="
echo "Instância: $INSTANCE_NAME"
echo "Arquivo: $INSTANCE_FILE"
echo "Método: $METHOD"
echo "Threads: $THREADS"
echo "Log será salvo em: $LOG_FILE"
echo "=================================="
echo ""

# Iniciar log com informações da execução
{
    echo "=== LOG DA INSTÂNCIA: $INSTANCE_NAME ==="
    echo "Início da execução: $(date '+%Y-%m-%d %H:%M:%S')"
    echo "Arquivo da instância: $INSTANCE_FILE"
    echo "Método: $METHOD"
    echo "Número de threads: $THREADS"
    echo "Sistema: $(uname -a)"
    echo "Usuário: $(whoami)"
    echo "Diretório de trabalho: $(pwd)"
    echo "======================================"
    echo ""
    
    # Executar o programa e capturar o tempo de execução
    echo "Iniciando execução do CBTU..."
    echo ""
    
    START_TIME=$(date '+%s')
    
    # Executar o programa
    $EXECUTABLE "$INSTANCE_FILE" "$METHOD" "$THREADS"
    EXIT_CODE=$?
    
    END_TIME=$(date '+%s')
    DURATION=$((END_TIME - START_TIME))
    
    echo ""
    echo "======================================"
    echo "Fim da execução: $(date '+%Y-%m-%d %H:%M:%S')"
    echo "Duração total: ${DURATION}s"
    echo "Código de saída: $EXIT_CODE"
    
    if [[ $EXIT_CODE -eq 0 ]]; then
        echo "Status: SUCESSO"
    else
        echo "Status: FALHA"
    fi
    
    echo "======================================"
    
} > "$LOG_FILE" 2>&1

# Mostrar resultado no terminal
echo "Execução finalizada!"
echo "Status: $([ $EXIT_CODE -eq 0 ] && echo "SUCESSO" || echo "FALHA")"
echo "Duração: ${DURATION}s"
echo "Log salvo em: $LOG_FILE"

# Mostrar últimas linhas do log se houver sucesso
if [[ $EXIT_CODE -eq 0 ]]; then
    echo ""
    echo "Últimas linhas do resultado:"
    echo "----------------------------"
    tail -10 "$LOG_FILE" | grep -v "^===" | grep -v "^Início\|^Fim\|^Duração\|^Status\|^Arquivo\|^Método\|^Threads\|^Sistema\|^Usuário\|^Diretório\|^Código"
fi

echo ""
echo "Para ver o log completo, execute:"
echo "cat $LOG_FILE"
