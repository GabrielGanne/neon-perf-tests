#!/bin/bash

for N in 8 16 32 64 128 256
do
    for FN in ethernet_frame_is_tagged_x2_ref ethernet_frame_is_tagged_x2_neon
    do
        echo "FN=$FN N=$N"
        make clean 1>/dev/null
        CFLAGS="-Dtest_fn=$FN -DN=$N" make 1>/dev/null
        perf stat -r 100 ./neon_ethernet_frame_is_tagged &> /tmp/neon_ethernet_frame_is_tagged.${FN}
    done
done
