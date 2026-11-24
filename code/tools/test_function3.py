#!/usr/bin/env python3
"""Run Function 3 and COW checks in an automatically removed D-drive build copy."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile


def main():
    source = Path(__file__).resolve().parents[1]
    objects = source / "obj"
    objects.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="function3-", dir=objects) as temp:
        copy = Path(temp) / "code"
        shutil.copytree(source, copy, ignore=shutil.ignore_patterns("obj", "bin", "__pycache__"))
        proc = copy / "kern/process/proc.c"
        text = proc.read_text().replace("#include <unistd.h>", "#include <unistd.h>\nstatic void f3_test(void);\nstatic void *f3_alloc_pcb(size_t);")
        text = text.replace("kmalloc(sizeof(struct proc_struct))", "f3_alloc_pcb(sizeof(struct proc_struct))", 1)
        text = text.replace("void\ncpu_idle(void)\n{", "void\ncpu_idle(void)\n{\n    f3_test();", 1)
        text += '\n#include "../../tools/test_function3.inc"\n'
        proc.write_text(text)
        proc.write_text(proc.read_text()
                        .replace("alloc_pages(KSTACKPAGE)", "f3_alloc_pages(KSTACKPAGE)")
                        .replace("(page = alloc_page())", "(page = f3_alloc_pages(1))"))
        pmm = copy / "kern/mm/pmm.c"
        pmm.write_text(pmm.read_text()
                       .replace("(page = alloc_page())", "(page = f3_alloc_pages(1))")
                       .replace("struct Page *copy = alloc_page();",
                                "struct Page *copy = f3_alloc_pages(1);"))
        vmm = copy / "kern/mm/vmm.c"
        vmm.write_text(vmm.read_text().replace("struct Page *copy = alloc_page();",
                                               "struct Page *copy = f3_alloc_pages(1);"))
        header = copy / "kern/mm/pmm.h"
        header.write_text(header.read_text() + "\nstruct Page *f3_alloc_pages(size_t n);\n")
        trap = copy / "kern/trap/trap.c"
        trap.write_text(trap.read_text().replace(
            "void exception_handler(struct trapframe *tf)\n{",
            "void f3_user_done(struct trapframe *tf);\n"
            "void exception_handler(struct trapframe *tf)\n{\n"
            "    if (tf->cause == CAUSE_USER_ECALL && tf->epc == UTEXT + 20) f3_user_done(tf);",
            1))
        # These callbacks only satisfy the isolated build. The probe uses the real
        # syscall dispatcher and must never call a test fallback.
        fallbacks = {
            "do_exit": "int do_exit(int code) { panic(\"unexpected exit %d\", code); }",
            "do_yield": "int do_yield(void) { panic(\"unexpected yield\"); }",
            "do_kill": "int do_kill(int pid) { panic(\"unexpected kill %d\", pid); }",
            "print_pgdir": "void print_pgdir(void) { panic(\"unexpected pgdir\"); }",
        }
        import re
        for name, filename in [("do_exit", proc), ("do_yield", proc),
                               ("do_kill", proc), ("print_pgdir", pmm)]:
            content = filename.read_text()
            if not re.search(r"\b(?:int|void)\s+" + name + r"\s*\([^;{}]*\)\s*\{", content):
                filename.write_text(content + "\n" + fallbacks[name] + "\n")
        build = subprocess.run(["make", "-j2"], cwd=copy, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        if build.returncode:
            print(build.stdout)
            return build.returncode
        qemu = os.environ.get("QEMU", "qemu-system-riscv64")
        command = [qemu, "-machine", "virt", "-nographic", "-bios", "default", "-device", "loader,file=bin/ucore.img,addr=0x80200000"]
        try:
            run = subprocess.run(command, cwd=copy, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=30)
            output = run.stdout
        except subprocess.TimeoutExpired as error:
            output = error.stdout or b""
            if isinstance(output, bytes): output = output.decode(errors="replace")
            print(output)
            print("FAIL: timeout")
            return 1
        print(output)
        return 0 if run.returncode == 0 and "[Lab5 F3] ALL TESTS PASS" in output and "kernel panic" not in output else 1


if __name__ == "__main__":
    raise SystemExit(main())
