# MPI+myth
MPI+myth is a high-performance MPI+ULT systems. MPI+myth focuses on implicit overlapping communication and computation.

## Requirement
- [MassiveThreads](https://github.com/massivethreads/massivethreads)
- MPI Library

## How to Run
```
make
make test
make run
```

```
cd benchs
mpiexec -n 2 ./conc_latency
MYTH_WORKERS_NUM=16 mpiexec -n 2 ./overlap 500
```

### miniFE

```
#!/bin/sh
#PBS -q u-interactive_4
#PBS -l select=4:mpiprocs=18
#PBS -W group_list=gc64
#PBS -l walltime=00:10:00
#PBS -N test
#PBS -j oe
#PBS -I
cd $PBS_O_WORKDIR
```

```
cd benchs/miniFE_ref_1.5
mpiexec -n 72 ./numa_test.sh 100
```

## References
Takuya Fukuoka, Wataru Endo, Kenjiro Taura:
An Efficient Inter-node Communication System with Lightweight-thread Scheduling.
HPCC 2019. Zhangjiajie, China. August 2018
