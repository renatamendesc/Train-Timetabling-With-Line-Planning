#!/bin/bash

# usage: ./run.sh <instances_folder> <method> <nb_threads> 

EXECUTABLE="./cbtu"

# check if method and number of threads was provided
if [[ $# -ne 3 ]]; then
    echo "Usage: $0 <instance_folder> <method> <nb_threads>"
    exit 1
fi

INSTANCES_FOLDER=$1
METHOD=$2
NB_THREADS=$3

# criar nome do log baseado nos parâmetros de entrada
LOG_FILE="results/results-${INSTANCES_FOLDER//\//-}-${METHOD}.log"

# validate method
if [[ "$METHOD" != "model" && "$METHOD" != "enum" && "$METHOD" != "heuristic" ]]; then
    echo "Error: Invalid method '$METHOD'"
    exit 1
fi

# check if folder exists
if [[ ! -d "$INSTANCES_FOLDER" ]]; then
    echo "Error: Instance folder '$INSTANCES_FOLDER' does not exist!"
    exit 1
fi

# check if the instances folder has .txt files
INSTANCES_FILES=("$INSTANCES_FOLDER"/*.txt)
if [[ ${#INSTANCES_FILES[@]} -eq 0 ]]; then
    echo "Error: No .txt files found in $INSTANCES_FOLDER!"
    exit 1
fi

# check if executable exists
if [[ ! -f "$EXECUTABLE" ]]; then
    echo "Error: Executable $EXECUTABLE not found!"
    exit 1
fi

# create or clear log file
echo "Results generated on $(date)" > "$LOG_FILE"
echo "Set of instances: $INSTANCES_FOLDER" >> "$LOG_FILE"
echo "Method: $METHOD" >> "$LOG_FILE"
echo "Number of threads: $NB_THREADS" >> "$LOG_FILE"
echo "-------------------------------------------" >> "$LOG_FILE"

# loop over each instance
for INSTANCE in "$INSTANCES_FOLDER"/*.txt; do
    if [[ -f "$INSTANCE" ]]; then
        INSTANCE_NAME=$(basename "$INSTANCE")

        echo -e "\nRunning $INSTANCE_NAME with method '$METHOD' and $NB_THREADS threads..."

        echo "$INSTANCE_NAME:" >> "$LOG_FILE"

        OUTPUT="$($EXECUTABLE "$INSTANCE" "$METHOD" "$NB_THREADS")"

        SOLUTION=$(awk -F'= ' '/-> Solution value =/ {split($2,a," "); print a[1]}' <<< "$OUTPUT")
        TIME=$(awk -F'= ' '/-> Total time =/ {print $2}' <<< "$OUTPUT")
        GAP=$(awk -F'= ' '/-> Gap value =/ {print $2}' <<< "$OUTPUT")
        TOTAL_COMB=$(grep -m1 "Total number of combinations =" <<< "$OUTPUT")
        FEASIBLE_COMB=$(grep -m1 "were feasible combination" <<< "$OUTPUT")

        if [[ "$METHOD" == "enum" ]]; then
            [[ -n "$TOTAL_COMB" ]] && echo "$TOTAL_COMB" >> "$LOG_FILE"
        fi

        echo "-> Solution value: $SOLUTION" >> "$LOG_FILE"
        echo "-> Total time: $TIME" >> "$LOG_FILE"
        echo "-> Gap value: $GAP" >> "$LOG_FILE"

        if [[ "$METHOD" == "enum" ]]; then
            [[ -n "$FEASIBLE_COMB" ]] && echo "$FEASIBLE_COMB" >> "$LOG_FILE"
        fi

        echo "" >> "$LOG_FILE"
    fi
done

echo -e "\nExecution finished! Results saved in $LOG_FILE"