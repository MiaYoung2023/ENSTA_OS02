#include <Eigen/Dense>  // Eigen 头文件
#include <cstdlib>
#include <vector>
#include <cassert>
#include <cmath>
#include <iostream>
#include <chrono>

using Eigen::MatrixXd;
using Eigen::VectorXd;

std::tuple<std::vector<double>, std::vector<double>,
           std::vector<double>, std::vector<double>> computeTensors(int dim)
{
    double pi = std::acos(-1.0);
    std::vector<double> u1(dim), u2(dim), v1(dim), v2(dim);

    for (int i = 0; i < dim; i++)
    {
        u1[i] = std::cos(1.67 * i * pi / dim);
        u2[i] = std::sin(2.03 * i * pi / dim + 0.25);
        v1[i] = std::cos(1.23 * i * i * pi / (7.5 * dim));
        v2[i] = std::sin(0.675 * i / (3.1 * dim));
    }
    return std::make_tuple(u1, u2, v1, v2);
}

// ✅ 修改: 直接返回 Eigen 矩阵
MatrixXd initTensorMatrices(const std::vector<double> &u, const std::vector<double> &v)
{
    int rows = u.size();
    int cols = v.size();
    MatrixXd A(rows, cols);
    
    for (int i = 0; i < rows; ++i)
        for (int j = 0; j < cols; ++j)
            A(i, j) = u[i] * v[j];
    
    return A;
}

// 计算向量点积
double dot(const std::vector<double> &u, const std::vector<double> &v)
{
    assert(u.size() == v.size());
    double scal = 0.0;
    for (size_t i = 0; i < u.size(); ++i)
        scal += u[i] * v[i];
    return scal;
}

// ✅ 修改: 适配 Eigen::MatrixXd
bool verifProduct(const std::vector<double> &uA, std::vector<double> &vA,
                  const std::vector<double> &uB, std::vector<double> &vB, const MatrixXd &C)
{
    double vAdotuB = dot(vA, uB);
    for (int i = 0; i < C.rows(); i++)
    {
        for (int j = 0; j < C.cols(); j++)
        {
            double rightVal = uA[i] * vAdotuB * vB[j];
            if (std::fabs(rightVal - C(i, j)) >
                100 * std::fabs(C(i, j) * std::numeric_limits<double>::epsilon()))
            {
                std::cerr << "Erreur numérique : valeur attendue pour C( " << i << ", " << j
                          << " ) -> " << rightVal << " mais valeur trouvée : " << C(i, j) << std::endl;
                return false;
            }
        }
    }
    return true;
}

int main(int nargs, char *vargs[])
{
    int dim = 1024;
    if (nargs > 1)
        dim = atoi(vargs[1]);

    std::vector<double> uA, vA, uB, vB;
    std::tie(uA, vA, uB, vB) = computeTensors(dim);

    // ✅ 直接使用 Eigen::MatrixXd 进行矩阵初始化
    MatrixXd A = initTensorMatrices(uA, vA);
    MatrixXd B = initTensorMatrices(uB, vB);

    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();
    
    // ✅ 关键: 使用 Eigen 进行矩阵乘法
    MatrixXd C = A * B;

    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;

    bool isPassed = verifProduct(uA, vA, uB, vB, C);
    if (isPassed)
    {
        std::cout << "Test passed\n";
        std::cout << "Temps CPU produit matrice-matrice Eigen : " << elapsed_seconds.count() << " secondes\n";
        std::cout << "MFlops -> " << (2. * dim * dim * dim) / elapsed_seconds.count() / 1e6 << std::endl;
    }
    else
    {
        std::cout << "Test failed\n";
    }

    return (isPassed ? EXIT_SUCCESS : EXIT_FAILURE);
}
