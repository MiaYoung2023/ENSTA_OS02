from mpi4py import MPI
import numpy as np
import time

# 初始化 MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

# 总样本数
nb_samples = 40_000_000
# 每个进程负责的样本数
local_samples = nb_samples // size

# 计时
start_time = time.time()

# 每个进程生成自己的随机点
x = 2. * np.random.random_sample(local_samples) - 1.
y = 2. * np.random.random_sample(local_samples) - 1.

# 计算落在单位圆内的点数
local_count = np.sum(x * x + y * y < 1.)

# 使用 MPI_Reduce 汇总所有进程的局部计数结果
global_count = comm.reduce(local_count, op=MPI.SUM, root=0)

# 进程 0 计算和输出最终结果
if rank == 0:
    approx_pi = 4. * global_count / nb_samples
    end_time = time.time()
    print(f"Temps pour calculer pi : {end_time - start_time} secondes")
    print(f"Pi vaut environ {approx_pi}")
