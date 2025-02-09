#include <iostream>
#include <random>
#include <omp.h>

#define NUM_POINTS 100000000  // 总点数
#define NUM_THREADS 8         // 线程数

double monte_carlo_pi(long num_points) {
    long count = 0;

    // 设置 OpenMP 线程数
    omp_set_num_threads(NUM_THREADS);

    // 直接在 for 循环上使用 reduction 以避免原子操作
    #pragma omp parallel 
    {
        std::random_device rd;
        std::mt19937 gen(rd() + omp_get_thread_num());  // 线程安全随机数生成器
        std::uniform_real_distribution<double> dist(0.0, 1.0);

        long local_count = 0;  // 线程私有的局部计数

        #pragma omp for nowait reduction(+:count)
        for (long i = 0; i < num_points; i++) {
            double x = dist(gen);
            double y = dist(gen);
            if (x * x + y * y <= 1.0) {
                local_count++;
            }
        }

        // 归并每个线程的计数
        count += local_count;
    }

    return 4.0 * count / num_points;
}

int main() {
    std::cout << "Monte Carlo Pi using OpenMP (C++)" << std::endl;

    double start_time = omp_get_wtime();  // 记录开始时间
    double pi = monte_carlo_pi(NUM_POINTS);
    double end_time = omp_get_wtime();  // 记录结束时间

    std::cout << "Estimated Pi = " << pi << std::endl;
    std::cout << "Execution Time = " << (end_time - start_time) << " seconds" << std::endl;

    return 0;
}
