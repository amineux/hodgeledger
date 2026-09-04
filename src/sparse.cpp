#include "hodgeledger/sparse.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace hodgeledger {
namespace {

void sort_and_coalesce(std::vector<Triplet>& trips) {
    std::sort(trips.begin(), trips.end(), [](const Triplet& a, const Triplet& b) {
        if (a.row != b.row) {
            return a.row < b.row;
        }
        return a.col < b.col;
    });
    std::vector<Triplet> out;
    out.reserve(trips.size());
    for (const Triplet& t : trips) {
        if (t.value == 0.0) {
            continue;
        }
        if (!out.empty() && out.back().row == t.row && out.back().col == t.col) {
            out.back().value += t.value;
            if (out.back().value == 0.0) {
                out.pop_back();
            }
        } else {
            out.push_back(t);
        }
    }
    trips.swap(out);
}

} // namespace

void SparseMatrix::multiply(std::span<const double> x, std::span<double> y) const {
    if (static_cast<int>(x.size()) != cols || static_cast<int>(y.size()) != rows) {
        throw std::runtime_error("sparse multiply: dimension mismatch");
    }
    for (int i = 0; i < rows; ++i) {
        double s = 0.0;
        for (int p = row_ptr[static_cast<std::size_t>(i)]; p < row_ptr[static_cast<std::size_t>(i + 1)];
             ++p) {
            s += values[static_cast<std::size_t>(p)] * x[static_cast<std::size_t>(col_idx[static_cast<std::size_t>(p)])];
        }
        y[static_cast<std::size_t>(i)] = s;
    }
}

double SparseMatrix::quadratic_form(std::span<const double> x) const {
    if (!square() || static_cast<int>(x.size()) != rows) {
        throw std::runtime_error("quadratic_form: expected square matrix matching x");
    }
    std::vector<double> y(static_cast<std::size_t>(rows), 0.0);
    multiply(x, y);
    double s = 0.0;
    for (int i = 0; i < rows; ++i) {
        s += x[static_cast<std::size_t>(i)] * y[static_cast<std::size_t>(i)];
    }
    return s;
}

SparseMatrix sparse_from_triplets(int rows, int cols, std::vector<Triplet> trips) {
    if (rows < 0 || cols < 0) {
        throw std::runtime_error("sparse_from_triplets: negative dimension");
    }
    sort_and_coalesce(trips);
    SparseMatrix A;
    A.rows = rows;
    A.cols = cols;
    A.row_ptr.assign(static_cast<std::size_t>(rows + 1), 0);
    A.col_idx.reserve(trips.size());
    A.values.reserve(trips.size());
    int cursor = 0;
    int t = 0;
    const int nt = static_cast<int>(trips.size());
    for (int r = 0; r < rows; ++r) {
        A.row_ptr[static_cast<std::size_t>(r)] = cursor;
        while (t < nt && trips[static_cast<std::size_t>(t)].row == r) {
            if (trips[static_cast<std::size_t>(t)].col < 0 || trips[static_cast<std::size_t>(t)].col >= cols) {
                throw std::runtime_error("sparse_from_triplets: column out of range");
            }
            A.col_idx.push_back(trips[static_cast<std::size_t>(t)].col);
            A.values.push_back(trips[static_cast<std::size_t>(t)].value);
            ++cursor;
            ++t;
        }
    }
    A.row_ptr[static_cast<std::size_t>(rows)] = cursor;
    return A;
}

SparseMatrix sparse_transpose(const SparseMatrix& A) {
    std::vector<Triplet> trips;
    trips.reserve(static_cast<std::size_t>(A.nnz()));
    for (int i = 0; i < A.rows; ++i) {
        for (int p = A.row_ptr[static_cast<std::size_t>(i)]; p < A.row_ptr[static_cast<std::size_t>(i + 1)];
             ++p) {
            trips.push_back({A.col_idx[static_cast<std::size_t>(p)], i, A.values[static_cast<std::size_t>(p)]});
        }
    }
    return sparse_from_triplets(A.cols, A.rows, std::move(trips));
}

SparseMatrix sparse_add(const SparseMatrix& A, const SparseMatrix& B) {
    if (A.rows != B.rows || A.cols != B.cols) {
        throw std::runtime_error("sparse_add: shape mismatch");
    }
    std::vector<Triplet> trips;
    trips.reserve(static_cast<std::size_t>(A.nnz() + B.nnz()));
    auto dump = [&](const SparseMatrix& M) {
        for (int i = 0; i < M.rows; ++i) {
            for (int p = M.row_ptr[static_cast<std::size_t>(i)]; p < M.row_ptr[static_cast<std::size_t>(i + 1)];
                 ++p) {
                trips.push_back({i, M.col_idx[static_cast<std::size_t>(p)], M.values[static_cast<std::size_t>(p)]});
            }
        }
    };
    dump(A);
    dump(B);
    return sparse_from_triplets(A.rows, A.cols, std::move(trips));
}

SparseMatrix gram_aat(const SparseMatrix& A) {
    // (A A^T)_{ik} = sum_j A_ij A_kj  — accumulate outer products of columns.
    const SparseMatrix At = sparse_transpose(A);
    std::vector<Triplet> trips;
    for (int j = 0; j < At.rows; ++j) {
        const int p0 = At.row_ptr[static_cast<std::size_t>(j)];
        const int p1 = At.row_ptr[static_cast<std::size_t>(j + 1)];
        for (int p = p0; p < p1; ++p) {
            const int i = At.col_idx[static_cast<std::size_t>(p)];
            const double ai = At.values[static_cast<std::size_t>(p)];
            for (int q = p0; q < p1; ++q) {
                const int k = At.col_idx[static_cast<std::size_t>(q)];
                const double ak = At.values[static_cast<std::size_t>(q)];
                trips.push_back({i, k, ai * ak});
            }
        }
    }
    return sparse_from_triplets(A.rows, A.rows, std::move(trips));
}

SparseMatrix gram_ata(const SparseMatrix& A) {
    return gram_aat(sparse_transpose(A));
}

} // namespace hodgeledger
