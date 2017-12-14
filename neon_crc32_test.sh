#!/bin/bash


for SZ in 4 8 16 32 64 128 256 512
do
    for FN in clib_crc32c clib_xxhash
    do
        for N in 1 2 4 8 16
        do
            echo "FN=$FN    SZ=$SZ    N=$N"
            make clean 1>/dev/null
            CFLAGS="-DBUFF_SZ=$SZ -Dhash_fn=$FN -DN=$N" make 1>/dev/null
            perf stat -r 10 ./neon_crc32 &> /tmp/neon_crc32.${FN}.${SZ}
        done
    done
done
