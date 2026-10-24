#!/usr/bin/env python3
"""Launch the existing make debug entry point and execute the guest GDB walk."""
import argparse
import os
from pathlib import Path
import signal
import socket
import subprocess
import time

parser = argparse.ArgumentParser()
parser.add_argument('--qemu', required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
os.chdir(root)
evidence = root.parent / 'report' / 'evidence'
with (evidence / 'sv39-qemu.log').open('w') as out:
    command = ['make', 'debug', 'QEMU=' + args.qemu]
    out.write('$ ' + ' '.join(command) + '\n')
    out.flush()
    proc = subprocess.Popen(command, stdout=out, stderr=subprocess.STDOUT,
                            start_new_session=True)
    try:
        for _ in range(100):
            if proc.poll() is not None:
                raise RuntimeError('make debug exited early')
            with socket.socket() as sock:
                if sock.connect_ex(('127.0.0.1', 1234)) == 0:
                    break
            time.sleep(0.05)
        else:
            raise TimeoutError('GDB port 1234 unavailable')
        with (evidence / 'sv39-gdb.log').open('w') as log:
            log.write('$ gdb-multiarch -q -batch -x tools/sv39.gdb\n')
            log.flush()
            subprocess.run(['gdb-multiarch', '-q', '-batch', '-x', 'tools/sv39.gdb'],
                           stdout=log, stderr=subprocess.STDOUT, timeout=30, check=True)
        assert '[Sv39] REAL GDB WALK PASSED' in (evidence / 'sv39-gdb.log').read_text()
    finally:
        if proc.poll() is None:
            os.killpg(proc.pid, signal.SIGTERM)
            proc.wait(timeout=10)
print('Sv39 guest GDB walk: PASS')
