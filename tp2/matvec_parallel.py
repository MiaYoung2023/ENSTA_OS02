import numpy as np
from mpi4py import MPI
import time

# 获取MPI通信的基本信息
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

# 初始化矩阵A和向量u
dim = 360
A = np.array([[(i + j) % dim + 1. for i in range(dim)] for j in range(dim)])
u = np.array([i + 1. for i in range(dim)])


# 每个进程处理的列数
Nloc = dim // size
start_time = time.time()
# 每个进程处理的矩阵列范围
start = rank * Nloc
end = (rank + 1) * Nloc if rank != size - 1 else dim
# 计算每个进程的部分乘积
## Produit par colonne
# partial_result = u.dot(A[:, start:end])

partial_result = A[start:end,:].dot(u)
# 如果是根进程，准备接收来自其他进程的结果
if rank == 0:
    result = np.zeros(dim)
    result[start:end] = partial_result
    # 根进程接收其他进程的部分结果
    for i in range(1, size):
        start_col_other = i * Nloc
        end_col_other = (i + 1) * Nloc if i != size - 1 else dim
        partial_result_received = comm.recv(source=i, tag=99)
        result[start_col_other:end_col_other] = partial_result_received
else:
    # 向根进程发送部分结果
    comm.send(partial_result, dest=0, tag=99)

# 根进程输出最终结果
if rank == 0:
    print(f"final result v = {result[:5]}... (5 elements shown)")
    parallel_time = time.time() - start_time  # 已完成并行计算
    # print(f"sequential_time: {sequential_time:.6f}s")
    print(f"parallel_time: {parallel_time:.6f}s")
    # speedup = sequential_time / parallel_time
    # print(f"speedup {speedup}")

