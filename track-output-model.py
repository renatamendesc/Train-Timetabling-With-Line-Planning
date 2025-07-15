import subprocess
import time
import re
import os

EXECUTABLE = "./cbtu"
INSTANCE_FOLDER = "instances"
OUTPUT_LOG = "results-optimal-time.log"

# capture best integer from lines starting with *
best_int_pattern = re.compile(r"\*.*?([0-9]+\.[0-9]+)")

# capture total time at the end of the output
total_time_pattern = re.compile(r"-> Total time = ([0-9]+\.[0-9]+)")

# get all instance files in the folder
instance_files = sorted([
    os.path.join(INSTANCE_FOLDER, f)
    for f in os.listdir(INSTANCE_FOLDER)
    if os.path.isfile(os.path.join(INSTANCE_FOLDER, f))
])

with open(OUTPUT_LOG, "w") as log_file:
    for instance in instance_files:
        instance_name = os.path.basename(instance)
        log_file.write(f"=== Running instance: {instance_name} ===\n")
        print(f"===> Running {instance_name}...")

        start_time = time.perf_counter()
        last_best_solution = None
        last_best_time = None
        final_total_time = None

        process = subprocess.Popen(
            [EXECUTABLE, instance],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1
        )

        for line in process.stdout:
            print(line, end='')

            # get new best solution
            if line.startswith('*'):
                elapsed = time.perf_counter() - start_time
                match = best_int_pattern.search(line)
                if match:
                    best_int = match.group(1)
                    last_best_solution = best_int
                    last_best_time = elapsed
                    log_file.flush()

            # get final total time
            match_time = total_time_pattern.search(line)
            if match_time:
                final_total_time = time.perf_counter() - start_time

        process.wait()

        # write results on .log file
        if last_best_solution is not None:
            log_file.write(f">>> Optimal ({last_best_solution}) was found at {last_best_time:.2f} seconds\n")
        else:
            log_file.write(">>> No integer solution was found.\n")

        if final_total_time is not None:
            log_file.write(f">>> Total time (CPLEX): {final_total_time:.2f} seconds\n")

        log_file.write("\n")
        log_file.flush()

print("All instances have been solved.")