#!/usr/bin/env python3
"""Use native GDB to debug the actual QEMU page walker and TLB installation."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--qemu', required=True, help='Unstripped QEMU 4.1.1 executable')
parser.add_argument('--bios-dir', required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
os.chdir(root)
symbols = subprocess.check_output(['riscv64-unknown-elf-nm', 'bin/kernel'], text=True)
va = next(int(line.split()[0], 16) for line in symbols.splitlines()
          if line.split()[-1] == 'kern_init')
command = ['gdb', '-q', '-nx', '-batch', '-ex', 'set $observed_va=0x%x' % va,
           '-x', 'tools/qemu_mmu.gdb', '--args', args.qemu, '-L', args.bios_dir,
           '-machine', 'virt', '-nographic', '-bios', 'default',
           '-device', 'loader,file=bin/ucore.img,addr=0x80200000']
path = root.parent / 'report' / 'evidence' / 'qemu-mmu-gdb.log'
with path.open('w') as out:
    out.write('$ ' + shlex.join(command) + '\n')
    out.flush()
    subprocess.run(command, stdout=out, stderr=subprocess.STDOUT,
                   timeout=60, check=True)
assert '[QEMU MMU] REAL HOST DEBUG PASSED' in path.read_text()
print('QEMU host MMU debug: PASS')
