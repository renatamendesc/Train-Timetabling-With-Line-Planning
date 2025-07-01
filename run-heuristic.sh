#!/bin/bash

# usage: ./run.sh [--ignore-real]
# example: ./run.sh --ignore-real

EXECUTABLE="./cbtu"
INSTANCES_FOLDER="./instances"
IGNORE_REAL=false
LOG_FILE="results.log"

# check if the number of threads was provided
if [[ -z "$1" || "$1" =~ ^-- ]]; then
    echo "Usage: $0 <nb_threads> [--ignore-real]"
    exit 1
fi

NB_THREADS=$1
shift

LOG_FILE="results/results-heuristic.log"

# parse additional flags
for arg in "$@"; do
    if [[ "$arg" == "--ignore-real" ]]; then
        IGNORE_REAL=true
    fi
done

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
echo "Number of threads: $NB_THREADS" >> "$LOG_FILE"
echo "Ignore '-real.txt' instances: $IGNORE_REAL" >> "$LOG_FILE"
echo "-------------------------------------------" >> "$LOG_FILE"

# loop over each instance
for INSTANCE in "$INSTANCES_FOLDER"/*.txt; do
    if [[ -f "$INSTANCE" ]]; then
        INSTANCE_NAME=$(basename "$INSTANCE")

        # Skip -real.txt if requested
        if [[ "$IGNORE_REAL" == true && "$INSTANCE_NAME" =~ -real\.txt$ ]]; then
            continue
        fi

        echo -e "\nRunning $INSTANCE_NAME with $NB_THREADS threads..."

        echo "$INSTANCE_NAME:" >> "$LOG_FILE"

        OUTPUT="$($EXECUTABLE "$INSTANCE")"
        SOLUTION=$(awk -F'= ' '/-> Solution value =/ {split($2,a," "); print a[1]}' <<< "$OUTPUT")
        TIME=$(awk -F'= ' '/-> Total time =/ {print $2}' <<< "$OUTPUT")

        echo "-> Solution value: $SOLUTION" >> "$LOG_FILE"
        echo "-> Total time: $TIME" >> "$LOG_FILE"

        echo "" >> "$LOG_FILE"
    fi
done

echo -e "\nExecution finished! Results saved in $LOG_FILE"