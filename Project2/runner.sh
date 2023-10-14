TIMEFORMAT=%R
# s | t | c | f
situation="-stcf"

echo -n '8 ' >&2
time ./a.out "${situation}" 1024 8
echo -n '16 ' >&2
time ./a.out "${situation}" 1024 16
echo -n '32 ' >&2
time ./a.out "${situation}" 1024 32
echo -n '64 ' >&2
time ./a.out "${situation}" 1024 64
echo -n '128 ' >&2
time ./a.out "${situation}" 1024 128
echo -n '256 ' >&2
time ./a.out "${situation}" 1024 256
echo -n '512 ' >&2
time ./a.out "${situation}" 1024 512
echo -n '1204 ' >&2
time ./a.out "${situation}" 1024 1024
echo -n '2048 ' >&2
time ./a.out "${situation}" 1024 2048
echo -n '4096 ' >&2
time ./a.out "${situation}" 1024 4096
echo -n '8128 ' >&2
time ./a.out "${situation}" 1024 8128
echo -n '10000 ' >&2
time ./a.out $situation 1024 10000
