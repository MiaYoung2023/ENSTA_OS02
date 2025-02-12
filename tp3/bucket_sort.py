from mpi4py import MPI
import numpy as np
from time import time
def bucket_sort(arr, nbp):
    # Step 1: Find the minimum and maximum values in the array
    min_value, max_value = np.min(arr), np.max(arr)
    
    # Step 2: Create nbp+1 quantiles to define the bucket ranges
    quantils = np.quantile(arr, np.linspace(0, 1, nbp + 1))
    # print(f"Quantils (bucket boundaries): {quantils}")

    # Step 3: Create empty buckets
    buckets = [[] for _ in range(nbp)]

    # Step 4: Distribute the elements into corresponding buckets
    for num in arr:
        # Find the appropriate bucket for the number
        for i in range(nbp):
            if quantils[i] <= num < quantils[i + 1]:
                buckets[i].append(num)
                break
    
    # Step 5: Sort the individual buckets
    sorted_buckets = [sorted(bucket) for bucket in buckets]
    
    # Step 6: Concatenate all the sorted buckets to get the final sorted array
    sorted_arr = np.concatenate(sorted_buckets)
    
    return sorted_arr

def main():
    # MPI setup
    comm = MPI.COMM_WORLD
    rank = comm.Get_rank()
    size = comm.Get_size()

    # Parameters
    N = 1000000 # Total number of elements
    nbp = size  # Number of buckets
    det = time()
    # Step 1: Generate data on rank 0 and distribute it
    if rank == 0:
        values = np.random.randint(-32768, 32768, size=N, dtype=int)
        # print(f"Initial values (rank 0): {values}")

        # Distribute chunks of the data to other processes
        chunk_size = N // size
        for i in range(1, size):
            comm.send(values[i * chunk_size: (i + 1) * chunk_size], dest=i)
        local_values = values[:chunk_size]
    else:
        # Other processes receive data
        local_values = comm.recv(source=0)

    # Step 2: Perform bucket sort locally on each process
    sorted_local_values = bucket_sort(local_values, nbp)

    # Step 3: Gather sorted local arrays on rank 0
    if rank == 0:
        # Collect data from all processes
        all_sorted_values = sorted_local_values
        for i in range(1, size):
            local_sorted = comm.recv(source=i)
            all_sorted_values = np.concatenate((all_sorted_values, sorted_local_values))
        
        # Final sorting of the concatenated results
        final_sorted = np.sort(all_sorted_values)
        # print(f"Final sorted values: {final_sorted}")
        fin = time()
        print(f"Time taken: {fin-det}")
    else:
        # Other processes send their sorted data back to rank 0
        comm.send(sorted_local_values, dest=0)

if __name__ == "__main__":
    main()
