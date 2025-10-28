#!/bin/bash

# Usage: ./benchmark.sh <instances_subfolder> <method> <threads>

# check input arguments
if [ $# -lt 3 ]; then
    echo "Usage: $0 <instances_subfolder> <method> <threads>"
    exit 1
fi

EXEC=./cbtu
SUBFOLDER=$(basename "$1")
METHOD=$2
THREADS=$3

INPUT_FOLDER=./instances/$SUBFOLDER
OUTPUT_FOLDER=./results/$SUBFOLDER/${METHOD}_${THREADS}
OUTPUT_FILE=$OUTPUT_FOLDER/benchmark.txt

# validate input folder
if [ ! -d "$INPUT_FOLDER" ]; then
    echo "Error: folder '$INPUT_FOLDER' not found."
    exit 1
fi

# prepare output folder
mkdir -p "$OUTPUT_FOLDER"
> "$OUTPUT_FILE"

# execute each instance
for f in "$INPUT_FOLDER"/*; do
    [ -f "$f" ] || continue
    INSTANCE=$(basename "$f")
    echo "Running: $INSTANCE | method=$METHOD | threads=$THREADS"

    SOL_LINE=$(echo "$OUTPUT" | grep "-> Solution value")
    TIME_LINE=$(echo "$OUTPUT" | grep "-> Total time")

    if echo "$OUTPUT" | grep -q "-> Gap value"; then
        GAP_LINE=$(echo "$OUTPUT" | grep "-> Gap value")
    else
        GAP_LINE="    -> Could not prove optimality!"
    fi

    # write result to output file
    {
        echo "$INSTANCE:"
        echo "    $SOL_LINE"
        echo "    $TIME_LINE"
        echo "    $GAP_LINE"
        echo ""
    } >> "$OUTPUT_FILE"
done