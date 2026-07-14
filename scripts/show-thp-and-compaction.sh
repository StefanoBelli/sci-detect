#/bin/bash

SYSFS_PSEUDO_FILES="
	/sys/kernel/mm/transparent_hugepage/enabled
	/sys/kernel/mm/transparent_hugepage/defrag
	/sys/kernel/mm/transparent_hugepage/shmem_enabled
	/sys/kernel/mm/transparent_hugepage/hugepages-*kB/enabled
	/sys/kernel/mm/transparent_hugepage/hugepages-*kB/shmem_enabled
"

PROCFS_COMPACTION="
	/proc/sys/vm/compaction_proactiveness
"

for file in $SYSFS_PSEUDO_FILES; do
	echo " *** $file *** "
	cat $file
	echo
	echo
done

echo " *** $PROCFS_COMPACTION *** "
cat $PROCFS_COMPACTION
