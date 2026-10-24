#!/usr/bin/env python3
"""Rebuild each configuration, run real QEMU, retain serial/build/grade logs."""
import argparse
import os
from pathlib import Path
import signal
import subprocess
import time

parser = argparse.ArgumentParser()
parser.add_argument('--qemu', required=True, help='QEMU command, including optional -L path')
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
evidence = root.parent / 'report' / 'evidence'
evidence.mkdir(parents=True, exist_ok=True)
os.chdir(root)


def build(name, defs=''):
    with (evidence / (name + '-build.log')).open('w') as out:
        for command in (['make', 'clean'], ['make', '-j4', 'DEFS=' + defs]):
            out.write('$ ' + ' '.join(command) + '\n')
            out.flush()
            subprocess.run(command, stdout=out, stderr=subprocess.STDOUT, check=True)


def boot(name, marker, defs=''):
    command = ['make', 'qemu', 'QEMU=' + args.qemu, 'DEFS=' + defs]
    path = evidence / (name + '-qemu.log')
    with path.open('w') as out:
        out.write('$ ' + ' '.join(command) + '\n')
        out.flush()
        proc = subprocess.Popen(command, stdout=out, stderr=subprocess.STDOUT,
                                start_new_session=True)
        try:
            deadline = time.monotonic() + 45
            while time.monotonic() < deadline:
                text = path.read_text(errors='replace')
                if 'panic' in text.lower() or 'assertion failed' in text.lower():
                    raise RuntimeError(name + ': kernel panic; see ' + str(path))
                if marker in text:
                    # Let subsequent startup code run before checking again.
                    time.sleep(0.3)
                    text = path.read_text(errors='replace')
                    assert 'panic' not in text.lower()
                    if not defs:
                        assert '[SLUB Test' not in text
                        assert 'memory management: best_fit_pmm_manager' in text
                    print(name + ': PASS', flush=True)
                    return
                if proc.poll() is not None:
                    raise RuntimeError(name + ': QEMU exited before success marker')
                time.sleep(0.05)
            raise TimeoutError(name + ': success marker not reached')
        finally:
            if proc.poll() is None:
                os.killpg(proc.pid, signal.SIGTERM)
                proc.wait(timeout=10)


build('slub', '-DSLUB_SELFTEST')
boot('slub', '[SLUB] ALL TESTS PASSED', '-DSLUB_SELFTEST')
build('default')
boot('default', 'satp physical address:')

# Keep official scripts intact. Preserve the serial output before make grade
# removes its temporary copy; stop its QEMU process after grading completes.
with (evidence / 'grade.log').open('w') as out:
    command = ['make', 'grade', 'QEMU=' + args.qemu]
    out.write('$ ' + ' '.join(command) + '\n')
    out.flush()
    proc = subprocess.Popen(command, stdout=out, stderr=subprocess.STDOUT,
                            start_new_session=True)
    try:
        deadline = time.monotonic() + 90
        while proc.poll() is None:
            serial = Path('/tmp/lab2_grade/.qemu.out')
            if serial.exists():
                try:
                    data = serial.read_bytes()
                    if data:
                        (evidence / 'grade-serial.log').write_bytes(data)
                except FileNotFoundError:
                    pass
            if time.monotonic() > deadline:
                raise TimeoutError('make grade exceeded 90 seconds')
            time.sleep(0.02)
        assert proc.returncode == 0
    finally:
        try:
            os.killpg(proc.pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
assert 'Total Score: 20/20' in (evidence / 'grade.log').read_text()
print('official grade: 20/20', flush=True)

build('buddy-slub', '-DUSE_BUDDY -DSLUB_SELFTEST')
boot('buddy-slub', '[SLUB] ALL TESTS PASSED', '-DUSE_BUDDY -DSLUB_SELFTEST')
text = (evidence / 'buddy-slub-qemu.log').read_text()
assert '[Buddy] ALL 8 TESTS PASSED' in text
assert 'memory management: buddy_pmm_manager' in text

# Leave the workspace in the default Best-Fit configuration for GDB and grading.
build('default-final')
boot('default-final', 'satp physical address:')
print('all configurations validated; default build restored', flush=True)
