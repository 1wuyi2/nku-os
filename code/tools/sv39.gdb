set pagination off
set confirm off
set architecture riscv:rv64
file bin/kernel
target remote localhost:1234
break slub_init
continue
info registers pc satp
python
import gdb
import re

MASK64 = (1 << 64) - 1
satp = int(gdb.parse_and_eval('$satp')) & MASK64
va = int(gdb.parse_and_eval('(unsigned long long)&kern_init')) & MASK64
assert satp >> 60 == 8, 'Sv39 must be enabled'
assert (va >> 39) == ((1 << 25) - 1 if va & (1 << 38) else 0)
base = (satp & ((1 << 44) - 1)) << 12
print('SATP=0x%016x MODE=%d ASID=%d ROOT_PA=0x%x' %
      (satp, satp >> 60, (satp >> 44) & 0xffff, base))
vpn = [(va >> (12 + 9 * level)) & 511 for level in range(3)]
print('VA=0x%016x VPN[2]=%d VPN[1]=%d VPN[0]=%d offset=0x%x' %
      (va, vpn[2], vpn[1], vpn[0], va & 4095))

def physical_word(pa):
    command = 'monitor xp /1gx 0x%x' % pa
    print('(gdb) ' + command)
    result = gdb.execute(command, to_string=True)
    print(result.strip())
    match = re.search(r':\s*(0x[0-9a-fA-F]+)', result)
    assert match, 'monitor physical read failed'
    return int(match.group(1), 16)

for level in (2, 1, 0):
    pte_pa = base + vpn[level] * 8
    pte = physical_word(pte_pa)
    flags = {name: (pte >> bit) & 1 for bit, name in enumerate('VRWXUGAD')}
    ppn = (pte >> 10) & ((1 << 44) - 1)
    print('L%d PTE_PA=0x%x PTE=0x%016x PPN=0x%x flags=%s' %
          (level, pte_pa, pte, ppn, flags))
    assert flags['V'] and not (flags['W'] and not flags['R'])
    assert pte >> 54 == 0, 'reserved upper PTE bits'
    if flags['R'] or flags['X']:
        assert flags['X'] and not flags['U'], 'kernel executable mapping'
        assert flags['A'], 'accessed mapping expected at this breakpoint'
        lower = (1 << (9 * level)) - 1
        assert ppn & lower == 0, 'misaligned superpage'
        page_size = 1 << (12 + 9 * level)
        pa = (ppn << 12) | (va & (page_size - 1))
        print('LEAF=L%d mapping_size=0x%x FINAL_PA=0x%x' % (level, page_size, pa))
        break
    assert level > 0 and not (pte & (0x10 | 0x40 | 0x80)), 'invalid non-leaf'
    base = ppn << 12
else:
    raise AssertionError('no leaf PTE found')

gdb.execute('x/8bx 0x%x' % va)
result = gdb.execute('monitor xp /8bx 0x%x' % pa, to_string=True)
print(result.strip())
physical = bytes(int(x, 16) for x in re.findall(r'0x([0-9a-fA-F]{2})\b', result))
virtual = bytes(gdb.selected_inferior().read_memory(va, 8))
assert physical == virtual
assert pa == va - 0xffffffff40000000
print('VERIFY: virtual bytes == physical bytes == %s; direct-map offset agrees' % virtual.hex())
print('[Sv39] REAL GDB WALK PASSED')
end
detach
quit
