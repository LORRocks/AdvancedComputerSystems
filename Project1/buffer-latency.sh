bufferSize=1
rm log.txt
for i in {1..100}; do
	./mlc --idle_latency -b${bufferSize}m -K0 > temp.txt
	echo -n ${bufferSize} >> log.txt
	echo -n ' , ' >> log.txt
	grep -oP '\((\s.+)\)' temp.txt >> log.txt
	((bufferSize=$bufferSize+1))
done
