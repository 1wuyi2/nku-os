#!/usr/bin/env python3
"""Build a separate creation-only kernel without implementing Function 3."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys


def main():
    source = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path,
                        default=source.parents[1] / "function2-check")
    parser.add_argument("--qemu", default=os.environ.get("QEMU", "qemu-system-riscv64"))
    parser.add_argument("--gcc-prefix", default="riscv64-unknown-elf-")
    args = parser.parse_args()
    output = args.output.resolve()
    repository = source.parent
    if output == repository or repository in output.parents or output in repository.parents:
        parser.error("output must be outside the source repository")
    output.mkdir(parents=True, exist_ok=True)
    sandbox = output / "code"
    if sandbox.exists():
        parser.error("output/code already exists; use a new --output directory")
    shutil.copytree(source, sandbox, ignore=shutil.ignore_patterns("obj", "bin", "__pycache__"))
    proc = sandbox / "kern/process/proc.c"
    text = proc.read_text()
    text = text.replace("kmalloc(sizeof(struct proc_struct))",
                        "function2_kmalloc(sizeof(struct proc_struct))", 1)
    text = text.replace("alloc_pages(KSTACKPAGE)", "function2_alloc_stack(KSTACKPAGE)", 1)
    text = text.replace("#include <assert.h>", "#include <assert.h>\n"
                        "static void *function2_kmalloc(size_t size);\n"
                        "static struct Page *function2_alloc_stack(size_t n);", 1)
    text = text.split("//LAB4 function3 YOUR CODE", 1)[0]
    text += '\n#include "../../tools/test_function2.inc"\n'
    proc.write_text(text)
    built = subprocess.run(["make", "-j2", "GCCPREFIX=" + args.gcc_prefix],
                           cwd=sandbox, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           text=True)
    (output / "build.log").write_text(built.stdout)
    if built.returncode:
        print(built.stdout)
        return built.returncode
    command = [args.qemu, "-machine", "virt", "-nographic", "-bios", "default",
               "-device", "loader,file=bin/ucore.img,addr=0x80200000"]
    try:
        run = subprocess.run(command, cwd=sandbox, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT, text=True, timeout=35)
        log = run.stdout
    except subprocess.TimeoutExpired as error:
        log = error.stdout or b""
        if isinstance(log, bytes):
            log = log.decode(errors="replace")
        (output / "run.log").write_text(log)
        print(log)
        print("FAIL: QEMU timed out", file=sys.stderr)
        return 1
    (output / "run.log").write_text(log)
    print(log)
    passed = (run.returncode == 0 and "[Function 2] ALL TESTS PASS" in log
              and "kernel panic" not in log and "FAIL" not in log)
    print("Evidence:", output)
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
