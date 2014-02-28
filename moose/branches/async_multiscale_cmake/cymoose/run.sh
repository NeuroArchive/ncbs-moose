#!/bin/bash -e
if [ $# -lt 1 ]; then
    rm -f ./moose_cython.so
    rm -f *.cpp
    CXX="g++" \
    CC="g++" \
    LDFLAGS="-L. -L/usr/lib -L/usr/local/lib -L/usr/lib/mpi/gcc/openmpi/lib/" \
    python ./setup.py build_ext --inplace
else
    echo "Just testing"
fi
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.
python test.py
