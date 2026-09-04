#pragma once

#include <span>
#include <vector>

namespace hodgeledger {

struct Triplet {
    int row = 0;
    int col = 0;
    double value = 0.0;
};

/// CSR matrix, possibly rectangular. `multiply` computes y = A x.
struct SparseMatrix {
    int rows = 0;
    int cols = 0;
    std::vector<int> row_ptr;
    std::vector<int> col_idx;
    std::vector<double> values;

    void multiply(std::span<const double> x, std::span<double> y) const;
    [[nodiscard]] double quadratic_form(std::span<const double> x) const;
    [[nodiscard]] int nnz() const noexcept { return static_cast<int>(values.size()); }
    [[nodiscard]] bool square() const noexcept { return rows == cols; }
};

SparseMatrix sparse_from_triplets(int rows, int cols, std::vector<Triplet> trips);
SparseMatrix sparse_transpose(const SparseMatrix& A);
SparseMatrix sparse_add(const SparseMatrix& A, const SparseMatrix& B);
SparseMatrix gram_aat(const SparseMatrix& A); // A A^T
SparseMatrix gram_ata(const SparseMatrix& A); // A^T A

} // namespace hodgeledger
