#ifndef SPARSEBENCH_EIGENUTILS_HPP
#define SPARSEBENCH_EIGENUTILS_HPP

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include "BenchmarkUtils.hpp"
#include "IO.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>


namespace sparsebench {
namespace eigenutils {

using EigenSparseMatrix = Eigen::SparseMatrix<double, Eigen::RowMajor, int32_t>;

EigenSparseMatrix readMatrix(const std::string path) {
  FILE *f = fopen(path.c_str(), "r");
  int m, n, nz, *ai, *aj;
  double *aval;
  MM_typecode matcode;
  mm_read_mtx_crd((char *) path.c_str(), &m, &n, &nz, &ai, &aj, &aval, &matcode);

  EigenSparseMatrix A(n, n);
  std::vector <Eigen::Triplet<double>> trips;

  for (size_t i = 0; i < nz; i++) {
    int row = ai[i] - 1;
    int col = aj[i] - 1;
    trips.push_back(Eigen::Triplet<double>(row, col, aval[i]));
    if (col != row)
      // TODO must check if matrix is symmetric
      trips.push_back(Eigen::Triplet<double>(col, row, aval[i]));
  }
  A.setFromTriplets(trips.begin(), trips.end());
  fclose(f);
  return A;
}

Eigen::VectorXd readVector(std::string path) {
  std::vector<double> v = sparsebench::io::readVector(path);
  Eigen::Map<Eigen::VectorXd> vec(v.data(), v.size());
  return vec;
}

template<typename SolverType>
void runSolver(int argc, char** argv) {
  using namespace std::chrono;

  sparsebench::benchmarkutils::parseArgs(argc, argv);

  auto A = sparsebench::eigenutils::readMatrix(argv[2]);
  auto b = sparsebench::eigenutils::readVector(argv[4]);
  auto exp = sparsebench::eigenutils::readVector(argv[6]);

  SolverType solver;

  auto t1 = high_resolution_clock::now();
  solver.compute(A);
  auto t2 = high_resolution_clock::now();
  auto setupSeconds = duration_cast<duration<double>>(t2 - t1).count();

  t1 = high_resolution_clock::now();
  int iterations = 2;
  Eigen::VectorXd x;
  for (int i = 0; i < iterations; i++) {
    x = solver.solve(b);
  }
  t2 = high_resolution_clock::now();
  auto solverPerIterationSeconds = duration_cast<duration<double>>(t2 - t1).count() / iterations;

  sparsebench::benchmarkutils::printSummary(
      setupSeconds,
      solver.iterations(),
      solverPerIterationSeconds,
      solver.error(),
      (x - exp).norm(),
      iterations
  );

}

}
}
#endif //SPARSEBENCH_EIGENUTILS_HPP
