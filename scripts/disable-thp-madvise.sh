#!/bin/bash

EXPAND_PRINT=0

if [ $UID -ne 0 ]; then
	echo "you must be root to do this"
	exit 1
fi

for_each_file_echo_never()
{
	vars=$@

	for var in $vars; do
		if [ $EXPAND_PRINT -eq 1 ]; then
			echo $var
		fi

		echo never > $var

		if [ $EXPAND_PRINT -eq 1 ]; then
			echo
			echo
		fi
	done
}

echo madvise > /sys/kernel/mm/transparent_hugepage/enabled
echo madvise > /sys/kernel/mm/transparent_hugepage/defrag
echo never > /sys/kernel/mm/transparent_hugepage/shmem_enabled

MTHP_HPS_ENABLED="/sys/kernel/mm/transparent_hugepage/hugepages-*kB/enabled"
for_each_file_echo_never $MTHP_HPS_ENABLED

MTHP_HPS_SHMEM_ENABLED="/sys/kernel/mm/transparent_hugepage/hugepages-*kB/shmem_enabled"
for_each_file_echo_never $MTHP_HPS_SHMEM_ENABLED

echo inherit > /sys/kernel/mm/transparent_hugepage/hugepages-2048kB/enabled
echo inherit > /sys/kernel/mm/transparent_hugepage/hugepages-2048kB/shmem_enabled
