#include <algorithm>
#include <cassert>
#include <iostream>
#include <thread>
#if defined(_OPENMP)
#include <omp.h>
#endif
#include "ProdMatMat.hpp"

namespace {
void prodSubBlocks(int iRowBlkA, int iColBlkB, int iColBlkA, int szBlock,
                   const Matrix& A, const Matrix& B, Matrix& C) {
  #pragma omp parallel for
  for (int j = iColBlkB; j < std::min(B.nbCols, iColBlkB + szBlock); j++)
    for (int k = iColBlkA; k < std::min(A.nbCols, iColBlkA + szBlock); k++)
      for (int i = iRowBlkA; i < std::min(A.nbRows, iRowBlkA + szBlock); ++i)
      
        C(i, j) += A(i, k) * B(k, j);
}
const int szBlock = 32;
}  // namespace

Matrix operator*(const Matrix& A, const Matrix& B) {
  Matrix C(A.nbRows, B.nbCols, 0.0);
  int szBlock = 1024;
  // prodSubBlocks(0, 0, 0, std::max({A.nbRows, B.nbCols, A.nbCols}), A, B, C);
  #pragma omp parallel for collapse(2)
  for (int i = 0; i < A.nbRows; i += szBlock) {    // 遍历 A 的行块
    for (int j = 0; j < B.nbCols; j += szBlock) {  // 遍历 B 的列块
      for (int k = 0; k < A.nbCols; k += szBlock) {// 遍历 A/B 的共享维度块
        prodSubBlocks(i, j, k, szBlock, A, B, C);
      }
    }
  }
  return C;
}
