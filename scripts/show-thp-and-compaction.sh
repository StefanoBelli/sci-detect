#/bin/bash

PSEUDO_FILES="
	/sys/kernel/mm/transparent_hugepage/enabled
	/sys/kernel/mm/transparent_hugepage/defrag
	/sys/kernel/mm/transparent_hugepage/shmem_enabled
	/sys/kernel/mm/transparent_hugepage/hugepages-*kB/enabled
	/sys/kernel/mm/transparent_hugepage/hugepages-*kB/shmem_enabled
	/proc/sys/vm/compaction_proactiveness
"

for file in $PSEUDO_FILES; do
	echo " *** $file *** "
	cat $file
	echo
	echo
done
