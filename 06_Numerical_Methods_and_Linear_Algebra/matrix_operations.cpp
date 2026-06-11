/*
 * ============================================================================
 * MODULE 06 — LESSON 1: Matrix Operations & Basic Linear Algebra
 * ============================================================================
 *
 * CONCEPT
 * -------
 * Linear algebra is the mathematical backbone of quantitative finance:
 *   - Portfolio variance:  σ²ₚ = wᵀ Σ w
 *   - Factor models:       R = α + β·F + ε
 *   - Cholesky decomposition for correlated Monte Carlo paths
 *
 * We build a minimal `Matrix` class from scratch (no external dependency)
 * to demonstrate:
 *   - Memory layout (row-major vs column-major)
 *   - Matrix-matrix and matrix-vector multiplication
 *   - Cholesky decomposition (positive semidefinite check)
 *   - Solving A·x = b via forward/backward substitution
 *
 * FINANCIAL EXAMPLE
 * -----------------
 * 1) Two-asset portfolio variance from a 2×2 covariance matrix.
 * 2) Cholesky decomposition of a correlation matrix.
 * 3) 3×3 linear system from a simple factor-model calibration.
 *
 * Mathematically:
 *   Σ = L·Lᵀ   (Cholesky)
 *   σ²ₚ = w₁²σ₁² + w₂²σ₂² + 2w₁w₂ρσ₁σ₂
 *
 * BUILD
 * -----
 *   cmake --build build --target matrix_operations
 *   ./build/06_Numerical_Methods_and_Linear_Algebra/matrix_operations
 * ============================================================================
 */

#include <cmath>
#include <cstddef>
#include <format>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace quant {

/// Minimal dense matrix (row-major) for pedagogical linear algebra.
class Matrix {
public:
    Matrix(std::size_t rows, std::size_t cols, double init = 0.0)
        : rows_(rows), cols_(cols), data_(rows * cols, init) {}

    [[nodiscard]] std::size_t rows() const noexcept { return rows_; }
    [[nodiscard]] std::size_t cols() const noexcept { return cols_; }

    double& operator()(std::size_t i, std::size_t j) {
        return data_[i * cols_ + j];
    }

    [[nodiscard]] double operator()(std::size_t i, std::size_t j) const {
        return data_[i * cols_ + j];
    }

    [[nodiscard]] const double* data() const noexcept { return data_.data(); }

    [[nodiscard]] Matrix transpose() const {
        Matrix result(cols_, rows_);
        for (std::size_t i = 0; i < rows_; ++i) {
            for (std::size_t j = 0; j < cols_; ++j) {
                result(j, i) = (*this)(i, j);
            }
        }
        return result;
    }

    void print(const std::string& label = "") const {
        if (!label.empty()) {
            std::cout << label << " (" << rows_ << "x" << cols_ << ")\n";
        }
        for (std::size_t i = 0; i < rows_; ++i) {
            std::cout << "  [";
            for (std::size_t j = 0; j < cols_; ++j) {
                std::cout << std::format("{:>8.4f}", (*this)(i, j));
                if (j + 1 < cols_) std::cout << ' ';
            }
            std::cout << "]\n";
        }
    }

private:
    std::size_t        rows_;
    std::size_t        cols_;
    std::vector<double> data_;
};

/// Matrix-matrix multiplication: C = A * B.
[[nodiscard]] Matrix mat_mul(const Matrix& A, const Matrix& B) {
    if (A.cols() != B.rows()) {
        throw std::invalid_argument("mat_mul: dimension mismatch");
    }
    Matrix C(A.rows(), B.cols(), 0.0);
    for (std::size_t i = 0; i < A.rows(); ++i) {
        for (std::size_t j = 0; j < B.cols(); ++j) {
            double sum = 0.0;
            for (std::size_t k = 0; k < A.cols(); ++k) {
                sum += A(i, k) * B(k, j);
            }
            C(i, j) = sum;
        }
    }
    return C;
}

/// Matrix-vector multiplication: y = A * x.
[[nodiscard]] std::vector<double> mat_vec_mul(const Matrix& A,
                                                const std::vector<double>& x) {
    if (A.cols() != x.size()) {
        throw std::invalid_argument("mat_vec_mul: dimension mismatch");
    }
    std::vector<double> y(A.rows(), 0.0);
    for (std::size_t i = 0; i < A.rows(); ++i) {
        double sum = 0.0;
        for (std::size_t j = 0; j < A.cols(); ++j) {
            sum += A(i, j) * x[j];
        }
        y[i] = sum;
    }
    return y;
}

/// Dot product of two vectors.
[[nodiscard]] double dot(const std::vector<double>& a,
                          const std::vector<double>& b) {
    double result = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        result += a[i] * b[i];
    }
    return result;
}

/// Cholesky decomposition: A = L * Lᵀ (A symmetric positive definite).
/// Returns lower triangular L. Throws if decomposition fails.
[[nodiscard]] Matrix cholesky(const Matrix& A) {
    if (A.rows() != A.cols()) {
        throw std::invalid_argument("cholesky: matrix must be square");
    }
    const std::size_t n = A.rows();
    Matrix L(n, n, 0.0);

    for (std::size_t j = 0; j < n; ++j) {
        double sum = 0.0;
        for (std::size_t k = 0; k < j; ++k) {
            sum += L(j, k) * L(j, k);
        }
        double val = A(j, j) - sum;
        if (val <= 1e-14) {
            throw std::runtime_error("cholesky: matrix not positive definite");
        }
        L(j, j) = std::sqrt(val);

        for (std::size_t i = j + 1; i < n; ++i) {
            double sum2 = 0.0;
            for (std::size_t k = 0; k < j; ++k) {
                sum2 += L(i, k) * L(j, k);
            }
            L(i, j) = (A(i, j) - sum2) / L(j, j);
        }
    }
    return L;
}

/// Solve L * x = b where L is lower triangular (forward substitution).
[[nodiscard]] std::vector<double> forward_subst(const Matrix& L,
                                                  const std::vector<double>& b) {
    const std::size_t n = L.rows();
    std::vector<double> x(n, 0.0);
    for (std::size_t i = 0; i < n; ++i) {
        double sum = 0.0;
        for (std::size_t j = 0; j < i; ++j) {
            sum += L(i, j) * x[j];
        }
        x[i] = (b[i] - sum) / L(i, i);
    }
    return x;
}

/// Solve Lᵀ * x = b where L is lower triangular (back substitution).
[[nodiscard]] std::vector<double> back_subst(const Matrix& L,
                                               const std::vector<double>& b) {
    const std::size_t n = L.rows();
    std::vector<double> x(n, 0.0);
    for (std::size_t i = n; i-- > 0;) {
        double sum = 0.0;
        for (std::size_t j = i + 1; j < n; ++j) {
            sum += L(j, i) * x[j];
        }
        x[i] = (b[i] - sum) / L(i, i);
    }
    return x;
}

/// Solve A * x = b via Cholesky: A = L*Lᵀ => L*y = b, Lᵀ*x = y.
[[nodiscard]] std::vector<double> solve_cholesky(const Matrix& A,
                                                   const std::vector<double>& b) {
    const Matrix L = cholesky(A);
    const std::vector<double> y = forward_subst(L, b);
    return back_subst(L, y);
}

/// Portfolio variance from weights and covariance matrix.
[[nodiscard]] double portfolio_variance(const std::vector<double>& weights,
                                          const Matrix& covar) {
    // σ²ₚ = wᵀ Σ w = wᵀ (Σ w)
    const std::vector<double> cov_w = mat_vec_mul(covar, weights);
    return dot(weights, cov_w);
}

}  // namespace quant

int main() {
    using namespace quant;

    std::cout << "=== Module 06: Matrix Operations & Linear Algebra ===\n\n";

    // -------------------------------------------------------------------------
    // 1. Matrix multiplication
    // -------------------------------------------------------------------------
    Matrix A(2, 3);
    A(0, 0) = 1.0; A(0, 1) = 2.0; A(0, 2) = 3.0;
    A(1, 0) = 4.0; A(1, 1) = 5.0; A(1, 2) = 6.0;

    Matrix B(3, 2);
    B(0, 0) = 7.0;  B(0, 1) = 8.0;
    B(1, 0) = 9.0;  B(1, 1) = 10.0;
    B(2, 0) = 11.0; B(2, 1) = 12.0;

    Matrix C = mat_mul(A, B);
    A.print("A");
    B.print("B");
    C.print("C = A * B");

    // -------------------------------------------------------------------------
    // 2. Portfolio variance from covariance matrix
    // -------------------------------------------------------------------------
    std::cout << "\n--- Portfolio Variance ---\n";
    Matrix covar(2, 2);
    covar(0, 0) = 0.04;   // σ₁² = 20%²
    covar(1, 1) = 0.09;   // σ₂² = 30%²
    covar(0, 1) = 0.024;  // ρ·σ₁·σ₂ = 0.4 * 0.2 * 0.3
    covar(1, 0) = 0.024;

    const std::vector<double> w{0.60, 0.40};
    const double var = portfolio_variance(w, covar);
    const double vol = std::sqrt(var);

    std::cout << std::format("Weights: [{:.2f}, {:.2f}]\n", w[0], w[1]);
    covar.print("Covariance matrix");
    std::cout << std::format("Portfolio variance : {:.6f}\n", var);
    std::cout << std::format("Portfolio vol      : {:.2f}%\n", vol * 100.0);

    // -------------------------------------------------------------------------
    // 3. Cholesky decomposition and linear solve
    // -------------------------------------------------------------------------
    std::cout << "\n--- Cholesky Decomposition ---\n";
    Matrix SPD(3, 3);
    SPD(0, 0) = 4.0;  SPD(0, 1) = 2.0;  SPD(0, 2) = 2.0;
    SPD(1, 0) = 2.0;  SPD(1, 1) = 5.0;  SPD(1, 2) = 3.0;
    SPD(2, 0) = 2.0;  SPD(2, 1) = 3.0;  SPD(2, 2) = 6.0;

    SPD.print("Symmetric positive definite A");

    Matrix L = cholesky(SPD);
    L.print("Cholesky factor L");

    // Verify: L * Lᵀ ≈ A
    Matrix recon = mat_mul(L, L.transpose());
    recon.print("L * Lᵀ (should equal A)");

    // Solve A * x = b
    const std::vector<double> b{8.0, 9.0, 10.0};
    std::vector<double> x = solve_cholesky(SPD, b);

    std::cout << "Solve A * x = b:\n";
    std::cout << "  b = [";
    for (std::size_t i = 0; i < b.size(); ++i) {
        std::cout << std::format("{:>6.2f}", b[i]);
        if (i + 1 < b.size()) std::cout << ", ";
    }
    std::cout << "]\n  x = [";
    for (std::size_t i = 0; i < x.size(); ++i) {
        std::cout << std::format("{:>8.4f}", x[i]);
        if (i + 1 < x.size()) std::cout << ", ";
    }
    std::cout << "]\n";

    // Verify A * x ≈ b
    std::vector<double> ax = mat_vec_mul(SPD, x);
    std::cout << "  A*x = [";
    for (std::size_t i = 0; i < ax.size(); ++i) {
        std::cout << std::format("{:>8.4f}", ax[i]);
        if (i + 1 < ax.size()) std::cout << ", ";
    }
    std::cout << "]\n";

    std::cout << "\nLinear algebra complete — next: root finding and IV.\n";

    return 0;
}
