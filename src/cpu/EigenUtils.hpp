#ifndef SPARSEBENCH_EIGENUTILS_HPP
#define SPARSEBENCH_EIGENUTILS_HPP

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include "BenchmarkUtils.hpp"
#include <chrono>
#include <iostream>

extern "C" {
#include <mmio.h>
}

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
    trips.push_back(Eigen::Triplet<double>(ai[i] - 1, aj[i] - 1, aval[i]));
  }
  A.setFromTriplets(trips.begin(), trips.end());
  std::cout << A << std::endl;
  fclose(f);
  return A;
}

Eigen::VectorXd readVector(std::string path) {
  FILE *f = fopen(path.c_str(), "r");
  int m, n;
  MM_typecode matcode;
  if (mm_read_banner(f, &matcode) != 0) {
    std::cout << "Error reading matrix banner" << std::endl;
    exit(1);
  }
  if (!mm_is_array(matcode)) {
    std::cout << "Expecting array in " << path << std::endl;
    exit(1);
  }
  mm_read_mtx_array_size(f, &m, &n);
  if (n != 1) {
    std::cout << "RHS should be column vector in " << path << std::endl;
    exit(1);
  }

  Eigen::VectorXd v(m);
  for (int i = 0; i < m; i++)
    fscanf(f, "%lf", &v[i]);

  fclose(f);
  return v;
}

template<typename SolverType>
void runSolver(int argc, char** argv) {
  using namespace std::chrono;
  sparsebench::benchmarkutils::parseArgs(argc, argv);

  auto A = sparsebench::eigenutils::readMatrix(argv[1]);
  auto b = sparsebench::eigenutils::readVector(argv[2]);

  SolverType solver;

  auto t1 = high_resolution_clock::now();
  solver.compute(A);
  auto t2 = high_resolution_clock::now();
  auto setupSeconds = duration_cast<duration<double>>(t2 - t1).count();

  t1 = high_resolution_clock::now();
  int iterations = 10;
  Eigen::VectorXd x;
  for (int i = 0; i < iterations; i++) {
    x = solver.solve(b);
  }
  t2 = high_resolution_clock::now();
  auto solverPerIterationSeconds = duration_cast<duration<double>>(t2 - t1).count() / iterations;

  sparsebench::benchmarkutils::printSummary(
      setupSeconds,
      solver.iterations(),
      solverPerIterationSeconds
  );
  std::cout << "solution: " << x << std::endl;
  std::cout << "estimated error: " << solver.error() << std::endl;
}

}
}
#endif //SPARSEBENCH_EIGENUTILS_HPP