#!/bin/bash

# Usage: ./benchmark.sh <instances_subfolder> <method> <threads> <solver> [language]
# language: "cpp" (default) or "python"

# check input arguments
if [ $# -lt 3 ]; then
    echo "Usage: $0 <instances_subfolder> <method> <threads> [language]"
    echo "  language: 'cpp' (default) or 'python'"
    exit 1
fi

SUBFOLDER=$(basename "$1")
METHOD=$2
THREADS=$3
SOLVER=$4
LANGUAGE=${5:-cpp}  # default to cpp if not provided

# validate language
if [ "$LANGUAGE" != "cpp" ] && [ "$LANGUAGE" != "python" ]; then
    echo "Error: language must be 'cpp' or 'python'"
    exit 1
fi

# set executable based on language
if [ "$LANGUAGE" = "python" ]; then
    # use python3 from venv if available, otherwise use system python3
    if [ -f "./python/venv/bin/python3" ]; then
        PYTHON_EXEC="./python/venv/bin/python3"
    else
        PYTHON_EXEC="python3"
    fi
    EXEC="$PYTHON_EXEC python/main.py"
    INPUT_FOLDER=./instances/$SUBFOLDER
    OUTPUT_FOLDER=./python/benchmarking/$SUBFOLDER/${METHOD}_${THREADS}_${SOLVER}
else
    EXEC=./cpp/cbtu
    INPUT_FOLDER=./instances/$SUBFOLDER
    OUTPUT_FOLDER=./cpp/benchmarking/$SUBFOLDER/${METHOD}_${THREADS}_${SOLVER}
fi

OUTPUT_FILE=$OUTPUT_FOLDER/benchmark-${SOLVER}.txt

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
    echo "Running: $INSTANCE | method=$METHOD | threads=$THREADS | solver=$SOLVER | language=$LANGUAGE"

    # create individual instance folder
    INSTANCE_FOLDER="$OUTPUT_FOLDER/$INSTANCE_NAME"
    mkdir -p "$INSTANCE_FOLDER"

    # execute the program and capture output
    OUTPUT=$($EXEC "$f" $METHOD $THREADS $SOLVER 2>&1)

    # save complete log to instance folder
    echo "$OUTPUT" > "$INSTANCE_FOLDER/output.log"

    TIME_LINE=$(echo "$OUTPUT" | grep "\-> Total time")
    SOL_LINE=$(echo "$OUTPUT" | grep "\-> Solution value")

    if echo "$OUTPUT" | grep -q "\-> Gap value"; then
        GAP_LINE=$(echo "$OUTPUT" | grep "\-> Gap value")
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

