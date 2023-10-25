
FILENAME='/dev/nvme0n1p3'
BLOCK_SIZE=(4K 32K 128K)
READ_WRITE=(100 70 0)
IO_DEPTH=(32 512 1024)


for bs in "${BLOCK_SIZE[@]}"
do
	BLOCK_SIZE=$bs
	for rw in "${READ_WRITE[@]}"
	do
		READ_WRITE=$rw
		for io in "${IO_DEPTH[@]}"
		do
			IO_DEPTH=$io
			echo "" >> results.txt
			echo "---------------------------------------------------------------------------------------------------------" >> results.txt
	echo "block size ${BLOCK_SIZE} read write ${READ_WRITE} io ${IO_DEPTH}"
	echo "block size ${BLOCK_SIZE} read write ${READ_WRITE} io ${IO_DEPTH}" >> results.txt
 sudo fio --filename=$FILENAME --direct=1 --rwmixread=$READ_WRITE --bs=$BLOCK_SIZE --ioengine=libaio --iodepth=$IO_DEPTH --runtime=30s --numjobs=4 --time_based --group_reporting --name=iops-test-job --eta-newline=1 >> results.txt
		done

	done
done

