#!/bin/bash

# usage: ./run.sh <method> <nb_threads> 

EXECUTABLE="./cbtu"
INSTANCES_FOLDER="./instances"
LOG_FILE="results/results-new.log"

# check if method and number of threads was provided
if [[ $# -ne 2 ]]; then
    echo "Usage: $0 <method> <nb_threads>"
    exit 1
fi

METHOD=$1
NB_THREADS=$2

# validate method
if [[ "$METHOD" != "model" && "$METHOD" != "enum" && "$METHOD" != "heuristic" ]]; then
    echo "Error: Invalid method '$METHOD'"
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
        # OPTIMAL_FOUND=$(awk -F'= ' '/-> Optimal was found =/ {print $2}' <<< "$OUTPUT")

        echo "-> Solution value: $SOLUTION" >> "$LOG_FILE"
        echo "-> Total time: $TIME" >> "$LOG_FILE"
        # echo "-> Optimal was found: $OPTIMAL_FOUND" >> "$LOG_FILE"

        echo "" >> "$LOG_FILE"
    fi
done

echo -e "\nExecution finished! Results saved in $LOG_FILE"