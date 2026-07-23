#!/bin/bash

# Usage: ./benchmark.sh <instances_subfolder> <method> <threads> <solver>

# check input arguments
if [ $# -lt 3 ]; then
    echo "Usage: $0 <instances_subfolder> <method> <threads>"
    exit 1
fi

SUBFOLDER=$(basename "$1")
METHOD=$2
THREADS=$3
SOLVER=$4

# use python3 from venv if available, otherwise use system python3
if [ -f "./python/venv/bin/python3" ]; then
    PYTHON_EXEC="./python/venv/bin/python3"
else
    PYTHON_EXEC="python3"
fi
EXEC="$PYTHON_EXEC python/main.py"
INPUT_FOLDER=./instances/$SUBFOLDER
OUTPUT_FOLDER=./python/benchmarking/$SUBFOLDER/${METHOD}_${THREADS}_${SOLVER}

OUTPUT_FILE=$OUTPUT_FOLDER/benchmark-${SOLVER}.txt
# OUTPUT_FILE=$OUTPUT_FOLDER/benchmark-${SOLVER}-test.txt

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
    INSTANCE_NAME="${INSTANCE%.*}"  # remove .txt
    echo "Running: $INSTANCE | method=$METHOD | threads=$THREADS | solver=$SOLVER"

    # create individual instance folder
    INSTANCE_FOLDER="$OUTPUT_FOLDER/$INSTANCE_NAME"
    mkdir -p "$INSTANCE_FOLDER"

    # execute the program and capture output
    OUTPUT=$($EXEC "$f" $METHOD $THREADS $SOLVER 2>&1)

    # save complete log to instance folder
    echo "$OUTPUT" > "$INSTANCE_FOLDER/output.log"

    TIME_LINE=$(echo "$OUTPUT" | grep "\-> Total time")
    SOL_LINE=$(echo "$OUTPUT" | grep "\-> Solution value")

    # build the optimality/gap line according to the method
    if [ "$METHOD" = "enum" ]; then
        # enumeration prints one of the two optimality messages
        GAP_LINE=$(echo "$OUTPUT" | grep -E "OPTIMAL \(optimality proven\)|Optimality could NOT be proven")
    elif [ "$METHOD" = "heuristic" ]; then
        # heuristic: optimality is never proven, so nothing is printed
        GAP_LINE=""
    else
        if echo "$OUTPUT" | grep -q "\-> Gap value"; then
            GAP_LINE=$(echo "$OUTPUT" | grep "\-> Gap value")
        else
            GAP_LINE="-> Could not prove optimality!"
        fi
    fi

    if [ "$METHOD" = "model" ]; then
        # solution.py prints "Lower bound" (lowercase b); match case-insensitively
        LB_LINE=$(echo "$OUTPUT" | grep -i "\-> Lower bound = " | head -n 1)
    else
        LB_LINE=""
    fi

    # write result to output file
    {
        echo "$INSTANCE:"
        echo "    $SOL_LINE"
        echo "    $TIME_LINE"
        if [ -n "$GAP_LINE" ]; then
            echo "    $GAP_LINE"
        fi
        if [ "$METHOD" = "model" ] && [ -n "$LB_LINE" ]; then
            echo "    $LB_LINE"
        fi
        echo ""
    } >> "$OUTPUT_FILE"
done

