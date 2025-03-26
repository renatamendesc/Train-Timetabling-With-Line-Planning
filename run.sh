#!/bin/bash

# execute './run.sh --ignore-real' if you would like to skip the real instances

EXECUTABLE="./cbtu"
INSTANCES_FOLDER="./instances"
IGNORE_REAL=false 

for arg in "$@"; do
    if [[ "$arg" == "--ignore-real" ]]; then
        IGNORE_REAL=true
        set -- "${@/--ignore-real}" 
    fi
done

# get the list of .txt files inside the folder
INSTANCES_FILES=("$INSTANCES_FOLDER"/*.txt)

# check if there are any .txt files
if [[ ${#INSTANCES_FILES[@]} -eq 0 ]]; then
    echo "Error: No files were found in $INSTANCES_FOLDER!"
    exit 1
fi

echo "Ignore '-real.txt' files: $IGNORE_REAL"

# check if executable exists
if [[ ! -f "$EXECUTABLE" ]]; then
    echo "Error: The executable $EXECUTABLE was not found!"
    exit 1
fi

for INSTANCE in "$INSTANCES_FOLDER"/*.txt; do
    if [[ -f "$INSTANCE" ]]; then
        # ignore real instances if flag was activated
        if [[ "$IGNORE_REAL" == true && "$INSTANCE" =~ -real\.txt$ ]]; then
            continue
        fi

        echo -e "\nRunning $EXECUTABLE with input $INSTANCE..."
        "$EXECUTABLE" "$INSTANCE"
    fi
done

echo -e "\nExecution completed!"