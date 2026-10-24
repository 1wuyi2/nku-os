set pagination off
set confirm off
set print thread-events off
handle SIGPIPE nostop noprint pass
break riscv_cpu_tlb_fill if mmu_idx == 3
run
printf "BARE/M-mode TLB miss: address=0x%lx mmu_idx=%d access=%d\n", address, mmu_idx, access_type
bt 6
disable 1
tbreak get_physical_address
continue
set $physical_out = physical
printf "BARE walker: addr=0x%lx satp=0x%lx priv=%lu\n", addr, env->satp, env->priv
finish
printf "BARE translated physical page=0x%lx\n", *$physical_out
break riscv_cpu_tlb_fill if address == $observed_va && mmu_idx == 1 && access_type == 2
continue
printf "SV39 TLB miss: address=0x%lx mmu_idx=%d access=%d\n", address, mmu_idx, access_type
bt 8
disable 3
tbreak get_physical_address
continue
set $physical_out = physical
printf "SV39 walker: addr=0x%lx satp=0x%lx priv=%lu\n", addr, env->satp, env->priv
tbreak cpu_helper.c:254
continue
printf "PTE read: base=0x%lx idx=%lu pte_addr=0x%lx pte=0x%lx ptshift=%d\n", base, idx, pte_addr, pte, ptshift
tbreak cpu_helper.c:333
continue
printf "Leaf: ppn=0x%lx ptshift=%d\n", ppn, ptshift
next
next
printf "Computed physical page=0x%lx\n", *$physical_out
finish
printf "Walker returned physical page=0x%lx\n", *$physical_out
if *$physical_out != (($observed_va - 0xffffffff40000000) & ~0xfff)
    quit 1
end
tbreak tlb_set_page
continue
printf "TLB install: VA=0x%lx PA=0x%lx prot=%d mmu_idx=%d size=0x%lx\n", vaddr, paddr, prot, mmu_idx, size
if vaddr != ($observed_va & ~0xfff) || paddr != (($observed_va - 0xffffffff40000000) & ~0xfff) || prot != 7 || mmu_idx != 1 || size != 4096
    quit 1
end
finish
tbreak cputlb.c:1030 if (addr & ~0xfff) == ($observed_va & ~0xfff) && env->priv == 1
continue
printf "TLB revisit: addr=0x%lx entry->addr_code=0x%lx\n", addr, entry->addr_code
if entry->addr_code != (addr & ~0xfff)
    quit 1
end
next
printf "TLB hit skips page walker; next source line:\n"
frame
printf "[QEMU MMU] REAL HOST DEBUG PASSED\n"
kill
quit
