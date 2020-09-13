# FastStat

FastStat is yet another fast resource statistics tool written in C.

## Build and Use

```
$ mkdir build
$ cd build
$ cmake .. # -DCMAKE_CUDA_HOST_COMPILER=/usr/bin/clang-8 may be necessary.
$ make
$ ./faststat
time,cpu.user,cpu.nice,cpu.sys,cpu.idle,cpu.iowait,cpu.irq,cpu.softirq,cpu.steal,mem.total,mem.used,mem.free,mem.shared,mem.buff_cache,mem.available,mem.swap.total,mem.swap.used,mem.swap.free,nvml.temp,nvml.power,nvml.usage,nvml.mem.used,nvml.mem.free,nvml.mem.total
2020-09-13 17:40:50.183,10.16,0.03,0.84,87.92,0.53,0.00,0.52,0.00,16313696,5353340,5209352,170100,5751004,10439348,2097148,251904,1845244,36,18.61,12,546.19,5390.38,5936.56
2020-09-13 17:40:50.689,1.50,0.00,0.17,98.33,0.00,0.00,0.00,0.00,16313696,5335372,5227320,170100,5751004,10457316,2097148,251904,1845244,37,18.72,5,546.19,5390.38,5936.56
2020-09-13 17:40:51.189,1.17,0.00,0.17,98.66,0.00,0.00,0.00,0.00,16313696,5333080,5229612,170100,5751004,10459608,2097148,251904,1845244,36,18.59,5,546.19,5390.38,5936.56
2020-09-13 17:40:51.688,3.51,0.00,1.34,94.82,0.00,0.00,0.33,0.00,16313696,5333240,5229440,170100,5751016,10459436,2097148,251904,1845244,37,18.57,9,546.19,5390.38,5936.56
2020-09-13 17:40:52.189,2.16,0.00,0.50,97.01,0.00,0.00,0.33,0.00,16313696,5333396,5231180,168204,5749120,10461188,2097148,251904,1845244,36,18.67,9,546.19,5390.38,5936.56
^C
$ ./faststat -t 0.1 time cpu.user mem.used
time,cpu.user,mem.used
2020-09-13 17:42:16.315,10.11,5360564
2020-09-13 17:42:16.418,4.10,5360564
2020-09-13 17:42:16.518,4.88,5361692
2020-09-13 17:42:16.618,6.45,5361208
2020-09-13 17:42:16.717,8.47,5361964
2020-09-13 17:42:16.814,8.00,5380916
2020-09-13 17:42:16.918,6.45,5361564
2020-09-13 17:42:17.017,5.00,5362076
^C
```
