# Memory protection keys

This is a feature of modern CPUs that allows the user to change protection
of associated PTEs not by changing PTEs themselves, but some registers 
instead! Of course, the benefit is that we don't need to do a memory store
anymore, we just write into a very fast CPU register.

Each CPU architecture may support this feature, Linux supports it.

## x86 CPUs

Intel introduced these in 2015 with Skylake. When a page fault occours and
it concerns protection key violation, the pushed-on-the-stack error code
is properly populated (```X86_PF_PK``` enabled).

See the intel manuals: https://cdrdv2-public.intel.com/922486/325384-092-sdm-vol-3abcd.pdf

## From the kernel perspective

Usage of PKU is at the request of the user via some system call (e.g.
```pkey_mprotect```).

The kernel, however may use this feature silently to implement **XOM** (eXecute Only Memory), that is, if you protect your memory (e.g. using ```mprotect``` or ```mmap```) with ```PROT_EXEC``` only, the kernel ensures you ONLY have that
enabled.

In x86, however, PTEs don't have a "no-read" protection bit, that is, as long
as the PTE is valid, you may always read from the memory it points to.

Now, if Linux detects that the CPU supports protection keys, Linux uses this
feature transparently to allow execution-only!

And segmentation faults take this into account: 
```SEGV_PKUERR``` and 
```force_sig_pkuerr```: 
https://elixir.bootlin.com/linux/v7.1.3/source/kernel/signal.c#L1762

Usage of ```arch_override_mprotect_pkey``` in ```do_mprotect_pkey```: https://elixir.bootlin.com/linux/v7.1.3/source/mm/mprotect.c#L931

Usage of ```execute_only_pkey``` in ```do_mmap```: https://elixir.bootlin.com/linux/v7.1.3/source/mm/mmap.c#L393

Source: https://docs.kernel.org/core-api/protection-keys.html

## How I ran into this

**NOT SURE THIS IS THE REAL CAUSE OF THE examples/ FAILURE (segfault error 4 insted of the expected error 25)**

**BUT IT MAY HAPPEN**

I got into this while running examples on a "newer" Intel CPU than my usual
CPU (the newer i7-1065G7 vs the older i7-6700K). Some examples **had** VMAs
with ```PROT_EXEC``` only. On the older CPU examples went fluid as expected (recall, 
even if no ```PROT_READ```, standard hw x86 PTEs allow reading anyway), on the newer CPU, SEGV!

Try an example: mmap with only PROT_EXEC, but do a read. error=0x25

## Support for memory protection keys in sci-detect

We will, in the future, need to support the fact that users may use
memory protection keys when CPU support the feature.
