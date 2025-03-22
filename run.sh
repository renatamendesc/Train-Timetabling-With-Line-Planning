#!/bin/bash

EXECUTABLE="./cbtu"
INSTANCES_FOLDER="./instances"

# get the list of .txt files inside the folder
INSTANCES_FILES=("$INSTANCES_FOLDER"/*.txt)

# check if there are any .txt files
if [[ ${#INSTANCES_FILES[@]} -eq 0 ]]; then
    echo "Error: No files were found in $INSTANCES_FOLDER!"
    exit 1
fi

# check if 2 parameters were passed, if not, all instances will be executed
if [[ $# -lt 2 ]]; then
    START="${INSTANCES_FILES[0]}"
    STOP="${INSTANCES_FILES[-1]}"
    echo "No parameters passed. Using:"
else
    START="$INSTANCES_FOLDER/$1"
    STOP="$INSTANCES_FOLDER/$2"
fi
echo "  Start instance: $(basename "$START")"
echo "  Stop instance: $(basename "$STOP")"

# check if executable and instances exist
if [[ ! -f "$EXECUTABLE" ]]; then
    echo "Error: The executable $EXECUTABLE was not found!"
    exit 1
fi
if [[ ! -f "$START" ]]; then
    echo "Error: The start instance '$START' does not exist!"
    exit 1
fi
if [[ ! -f "$STOP" ]]; then
    echo "Error: The stop instance '$STOP' does not exist!"
    exit 1
fi

RUNNING=false # variable that tells whether start instance was already found
for INSTANCE in "$INSTANCES_FOLDER"/*.txt; do
    if [[ -f "$INSTANCE" ]]; then
        if [[ "$INSTANCE" == "$START" ]]; then
            RUNNING=true
            echo "Starting execution from $INSTANCE..."
        fi

        # execute instances after start was found
        if [[ "$RUNNING" == true ]]; then
            echo -e "\nRunning $EXECUTABLE with input $INSTANCE..."
            "$EXECUTABLE" "$INSTANCE"
        fi

        # finishes execution stop instanc was found
        if [[ "$INSTANCE" == "$STOP" ]]; then
            break
        fi
    fi
done

echo -e "\nExecution completed!"