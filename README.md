# FastStat

FastStat is yet another fast resource statistics tool written in C.

## Build and Use

```
$ mkdir build
$ cd build
$ cmake .. # -DCMAKE_CUDA_HOST_COMPILER=/usr/bin/clang-8 may be necessary.
$ make
$ ./faststat
time,cpu.user,cpu.nice,cpu.sys,cpu.idle,cpu.iowait,cpu.irq,cpu.softirq,nvml.temp,nvml.power,nvml.usage,nvml.mem.used,nvml.mem.free,nvml.mem.total
2020-05-11 22:12:34.712,0.00,0.00,100.00,0.00,0.00,100.00,100.00,33,18.77,4,413.81,5522.75,5936.56
2020-05-11 22:12:35.214,0.83,0.00,0.50,98.50,0.00,0.00,0.17,33,18.67,5,413.81,5522.75,5936.56
2020-05-11 22:12:35.716,0.67,0.00,0.67,98.66,0.00,0.00,0.00,33,18.68,5,413.81,5522.75,5936.56
2020-05-11 22:12:36.217,3.66,0.00,0.83,95.17,0.17,0.00,0.17,33,18.72,8,413.81,5522.75,5936.56
```
