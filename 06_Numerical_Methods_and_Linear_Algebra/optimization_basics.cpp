/*
 * ============================================================================
 * MODULE 06 — LESSON 3: Optimization Basics — Gradient Descent & Calibration
 * ============================================================================
 *
 * CONCEPT
 * -------
 * Many quant problems reduce to minimizing a loss function:
 *
 *   θ* = argmin_θ  Σᵢ (model_priceᵢ(θ) - market_priceᵢ)²
 *
 * Gradient descent iteratively steps downhill:
 *
 *   θ_{k+1} = θ_k - α · ∇L(θ_k)
 *
 * where α is the learning rate and ∇L is the gradient of the loss.
 *
 * FINANCIAL EXAMPLE
 * -----------------
 * 1) Minimize a simple convex function (f(x) = x² + 2) as warmup.
 * 2) Calibrate a linear factor model: R_i = β·F + ε
 *    — minimize MSE(β) = (1/N) Σ (R_i - β·F_i)²
 * 3) Two-parameter curve calibration (simple yield-curve fitting).
 *
 * BUILD
 * -----
 *   cmake --build build --target optimization_basics
 *   ./build/06_Numerical_Methods_and_Linear_Algebra/optimization_basics
 * ============================================================================
 */

#include <cmath>
#include <format>
#include <iostream>
#include <numbers>
#include <random>
#include <vector>

namespace quant {

/// Gradient descent for a single-parameter convex function.
[[nodiscard]] double gradient_descent_1d(
    double (*f)(double),
    double (*grad)(double),
    double x0,
    double learning_rate = 0.1,
    int    max_iter      = 1000,
    double tol           = 1e-10) {

    double x = x0;
    for (int i = 0; i < max_iter; ++i) {
        const double g = grad(x);
        x -= learning_rate * g;
        if (std::abs(g) < tol) {
            break;
        }
    }
    return x;
}

/// Calibrate β in R_i = β·F_i + ε via closed form (most efficient).
/// β = (Σ R_i·F_i) / (Σ F_i²)
[[nodiscard]] double calibrate_beta(const std::vector<double>& R,
                                     const std::vector<double>& F) {
    double num = 0.0;
    double den = 0.0;
    for (std::size_t i = 0; i < R.size(); ++i) {
        num += R[i] * F[i];
        den += F[i] * F[i];
    }
    if (std::abs(den) < 1e-15) {
        return 0.0;
    }
    return num / den;
}

/// MSE loss for β calibration (for gradient descent demo).
[[nodiscard]] double mse_loss(const std::vector<double>& R,
                               const std::vector<double>& F,
                               double beta) {
    double sum = 0.0;
    for (std::size_t i = 0; i < R.size(); ++i) {
        const double err = R[i] - beta * F[i];
        sum += err * err;
    }
    return sum / static_cast<double>(R.size());
}

/// d(MSE)/dβ = (-2/N) Σ (R_i - β·F_i)·F_i
[[nodiscard]] double mse_gradient(const std::vector<double>& R,
                                   const std::vector<double>& F,
                                   double beta) {
    double sum = 0.0;
    for (std::size_t i = 0; i < R.size(); ++i) {
        sum += (R[i] - beta * F[i]) * F[i];
    }
    return -2.0 * sum / static_cast<double>(R.size());
}

/// Two-parameter yield curve: y(t; a, b) = a + b·t
/// Fit to observed market yields at various maturities.
struct YieldCurveParams {
    double a;  // intercept
    double b;  // slope
};

/// Model yield at tenor t.
[[nodiscard]] double model_yield(double t, double a, double b) {
    return a + b * t;
}

/// MSE loss for yield curve calibration.
[[nodiscard]] double yc_loss(const std::vector<double>& tenors,
                              const std::vector<double>& market_yields,
                              double a, double b) {
    double sum = 0.0;
    for (std::size_t i = 0; i < tenors.size(); ++i) {
        const double err = model_yield(tenors[i], a, b) - market_yields[i];
        sum += err * err;
    }
    return sum / static_cast<double>(tenors.size());
}

/// Simple grid search for two-parameter optimization.
[[nodiscard]] YieldCurveParams grid_search_yc(
    const std::vector<double>& tenors,
    const std::vector<double>& market_yields,
    double a_min, double a_max, int a_steps,
    double b_min, double b_max, int b_steps) {

    double best_loss = std::numeric_limits<double>::max();
    YieldCurveParams best{0.0, 0.0};

    const double da = (a_max - a_min) / static_cast<double>(a_steps);
    const double db = (b_max - b_min) / static_cast<double>(b_steps);

    for (int ai = 0; ai <= a_steps; ++ai) {
        const double a = a_min + static_cast<double>(ai) * da;
        for (int bi = 0; bi <= b_steps; ++bi) {
            const double b = b_min + static_cast<double>(bi) * db;
            const double loss = yc_loss(tenors, market_yields, a, b);
            if (loss < best_loss) {
                best_loss = loss;
                best = {a, b};
            }
        }
    }
    return best;
}

}  // namespace quant

int main() {
    using namespace quant;

    std::cout << "=== Module 06: Optimization Basics ===\n\n";

    // -------------------------------------------------------------------------
    // 1. Minimize f(x) = x² + 2 (minimum at x = 0)
    // -------------------------------------------------------------------------
    const auto f    = [](double x) { return x * x + 2.0; };
    const auto grad = [](double x) { return 2.0 * x; };

    const double x_opt = gradient_descent_1d(f, grad, 5.0);
    std::cout << "1. Minimize f(x) = x² + 2\n";
    std::cout << std::format("   Starting from x0 = 5.0\n");
    std::cout << std::format("   Minimum at x = {:.8f}  (expected 0)\n", x_opt);
    std::cout << std::format("   f(x) = {:.8f}\n\n", f(x_opt));

    // -------------------------------------------------------------------------
    // 2. Calibrate linear factor model β
    // -------------------------------------------------------------------------
    std::cout << "2. Linear factor model calibration\n";

    std::mt19937 rng{42};
    std::normal_distribution<double> noise{0.0, 0.02};
    std::uniform_real_distribution<double> f_dist{-1.0, 1.0};

    constexpr std::size_t n_obs = 50;
    constexpr double true_beta = 1.25;

    std::vector<double> R(n_obs);
    std::vector<double> F(n_obs);
    for (std::size_t i = 0; i < n_obs; ++i) {
        F[i] = f_dist(rng);
        R[i] = true_beta * F[i] + noise(rng);
    }

    const double beta_ls = calibrate_beta(R, F);
    std::cout << std::format("   True β     : {:.4f}\n", true_beta);
    std::cout << std::format("   LS estimate: {:.4f}\n", beta_ls);
    std::cout << std::format("   MSE at β   : {:.6e}\n", mse_loss(R, F, beta_ls));

    // Gradient descent for β starting from 0
    double beta_gd = 0.0;
    for (int iter = 0; iter < 500; ++iter) {
        beta_gd -= 0.5 * mse_gradient(R, F, beta_gd);
    }
    std::cout << std::format("   GD estimate: {:.4f}\n\n", beta_gd);

    // -------------------------------------------------------------------------
    // 3. Yield curve calibration via grid search
    // -------------------------------------------------------------------------
    std::cout << "3. Yield curve calibration (grid search)\n";

    const std::vector<double> tenors{1.0, 2.0, 3.0, 5.0, 7.0, 10.0};
    const std::vector<double> market_yields{0.035, 0.038, 0.042, 0.045, 0.046, 0.048};

    const YieldCurveParams best = grid_search_yc(
        tenors, market_yields,
        0.0, 0.06, 200,
        0.0, 0.005, 200);

    std::cout << std::format("   Model: y(t) = a + b·t\n");
    std::cout << std::format("   a (intercept) : {:.6f}\n", best.a);
    std::cout << std::format("   b (slope)     : {:.6f}\n", best.b);
    std::cout << std::format("   Final MSE     : {:.2e}\n",
                             yc_loss(tenors, market_yields, best.a, best.b));

    std::cout << "\n   Calibrated yields:\n";
    std::cout << std::format("{:>8} {:>10} {:>12}\n", "Tenor", "Market", "Model");
    for (std::size_t i = 0; i < tenors.size(); ++i) {
        const double y_model = model_yield(tenors[i], best.a, best.b);
        std::cout << std::format("{:>8.1f} {:>10.4f} {:>12.4f}\n",
                                 tenors[i], market_yields[i], y_model);
    }

    std::cout << "\nOptimization techniques enable calibration to market data.\n"
                 "Next: derivative pricing models with trees, BS, and MC.\n";

    return 0;
}
