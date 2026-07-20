import sys
from pathlib import Path

# repository root (this file lives in <repo>/python)
REPO_ROOT = Path(__file__).resolve().parent.parent

# every run stores its outputs next to the benchmark files:
# python/benchmarking/<instance_set>/<method>_<threads>_<solver>/<instance_name>/
#   ├── output.log
#   ├── timetable.txt
#   └── graph.png
BENCHMARKING_ROOT = REPO_ROOT / "python" / "benchmarking"


def run_output_dir(data, method, threads, solver):
    out_dir = (
        BENCHMARKING_ROOT
        / data.instance_set
        / f"{method}_{threads}_{solver}"
        / data.instance_name
    )
    out_dir.mkdir(parents=True, exist_ok=True)
    return out_dir


class Tee:
    def __init__(self, *streams):
        self.streams = streams

    def write(self, data):
        for stream in self.streams:
            stream.write(data)
            stream.flush()

    def flush(self):
        for stream in self.streams:
            stream.flush()


def log_file_path(data, method, threads, solver):
    return run_output_dir(data, method, threads, solver) / "output.log"


def setup_log_file(log_path):
    log_fp = open(log_path, "w", encoding="utf-8")
    sys.stdout = Tee(sys.__stdout__, log_fp)
    sys.stderr = Tee(sys.__stderr__, log_fp)
    return log_fp


def close_log_file(log_fp, log_path):
    sys.stdout = sys.__stdout__
    sys.stderr = sys.__stderr__
    log_fp.close()
    print(f"Log saved to {log_path}")
