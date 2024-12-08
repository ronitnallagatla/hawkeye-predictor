# Hawkeye Predictor

## Build

``` console
scons-3 USE_HDF5=0 -j `nproc` ./build/ECE565-ARM/gem5.opt
```

## Run SPEC Benchmarks

``` console
./build/ECE565-ARM/gem5.opt configs/spec/spec_se.py --cpu-type=MinorCPU -b <benchmark-name> --cache --l2cache --l3cache --l3_rp=hawkeye --maxinsts=10000000
```
---