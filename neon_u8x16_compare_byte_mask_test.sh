#!/bin/bash

for FN in u8x16_compare_byte_mask_ref u8x16_compare_byte_mask_test
do
    for N in 1 2 4 8 16
    do
        echo "FN=$FN    N=$N"
        make clean 1>/dev/null
        CFLAGS="-Dfn=$FN -DN=$N" make 1>/dev/null
        perf stat -r 10 ./neon_u8x16_compare_byte_mask &> /tmp/neon_u8x16_compare_byte_mask.${FN}.${N}
    done
done
