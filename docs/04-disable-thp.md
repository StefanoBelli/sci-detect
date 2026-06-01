# Disabling Transparent hugepages (THP)

This feature allows for reduction in amount of TLB entries, 
therefore reducing costly page table walks by the MMU.

This is done transparently*

Khugepaged directly affected, no need to change anything in 
```/sys/kernel/mm/transparent_hugepage/khugepaged/``` subdir

## Turn-off completely

Kernel will never attempt to transparently "merge" PTEs transparently or other stuff...

```bash
#!/bin/bash

echo never > /sys/kernel/mm/transparent_hugepage/enabled
echo never > /sys/kernel/mm/transparent_hugepage/defrag
echo never > /sys/kernel/mm/transparent_hugepage/shmem_enabled
echo never > /sys/kernel/mm/transparent_hugepage/hugepages-*kB/enabled
echo never > /sys/kernel/mm/transparent_hugepage/hugepages-*kB/shmem_enabled
echo inherit > /sys/kernel/mm/transparent_hugepage/hugepages-2048kB/enabled
echo inherit > /sys/kernel/mm/transparent_hugepage/hugepages-2048kB/shmem_enabled
```

## Leaving the possibility to the apps of madvising the kernel for THP

Kernel will try to do THP when application requests to do so (via ```madvise``` system call)

```bash
#!/bin/bash

echo madvise > /sys/kernel/mm/transparent_hugepage/enabled
echo madvise > /sys/kernel/mm/transparent_hugepage/defrag
echo never > /sys/kernel/mm/transparent_hugepage/shmem_enabled
echo never > /sys/kernel/mm/transparent_hugepage/hugepages-*kB/enabled
echo never > /sys/kernel/mm/transparent_hugepage/hugepages-*kB/shmem_enabled
echo inherit > /sys/kernel/mm/transparent_hugepage/hugepages-2048kB/enabled
echo inherit > /sys/kernel/mm/transparent_hugepage/hugepages-2048kB/shmem_enabled
```

# Disabling explicit hugepages 

This is the situation where a user may explicitly request a 2MB (even 1GB?? not sure...) huge page to themselves.

Normally, the kernel will not allow the above-mentioned situation (due to fast memory exhaustion?),
and on a normal workload the number of (globally) assignable huge pages to user process is 0
(default on most distros, I suppose).

However, the sysadmin may choose to allow them up to a certain number.

### 3 ways to check

* Via ```/proc/meminfo```
```grep -i HugePages_Total /proc/meminfo```

* Via ```/proc/sys/vm/nr_hugepages```
```cat /proc/sys/vm/nr_hugepages```

* Via sysctl options
```sysctl vm.nr_hugepages```

#### Sources
 * https://access.redhat.com/solutions/46111
 * https://publish.obsidian.md/mm/Transparent+Huge+Pages+(THP)

