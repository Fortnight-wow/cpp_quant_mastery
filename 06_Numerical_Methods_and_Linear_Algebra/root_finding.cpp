/*
 * ============================================================================
 * MODULE 06 — LESSON 2: Root Finding & Implied Volatility
 * ============================================================================
 *
 * CONCEPT
 * -------
 * Root finding solves f(x) = 0. In quantitative finance its most famous
 * application is **implied volatility inversion**:
 *
 *   Given market price C_mkt, find σ such that:
 *     f(σ) = C_BS(S, K, r, σ, T) - C_mkt = 0
 *
 * Three methods (in increasing robustness):
 *   1) Bisection       — guaranteed convergence, linear rate
 *   2) Newton–Raphson  — quadratic convergence but requires derivative
 *   3) Brent's method  — combines bisection + inverse quadratic interpolation
 *
 * FINANCIAL EXAMPLE
 * -----------------
 * We have a European call trading at $6.50 with:
 *   S=100, K=105, r=0.03, T=0.5
 *
 * We invert the Black-Scholes formula to find the implied volatility σ_imp.
 *
 * For Newton–Raphson we need vega (∂C/∂σ):
 *
 *   vega = S · N'(d₁) · √T
 *   N'(d₁) = exp(-d₁²/2) / √(2π)
 *
 * BUILD
 * -----
 *   cmake --build build --target root_finding
 *   ./build/06_Numerical_Methods_and_Linear_Algebra/root_finding
 * ============================================================================
 */

#include <cmath>
#include <format>
#include <iostream>
#include <numbers>
#include <stdexcept>

namespace quant {

// -----------------------------------------------------------------------------
// Black-Scholes components
// -----------------------------------------------------------------------------

[[nodiscard]] double norm_cdf(double x) {
    return 0.5 * (1.0 + std::erf(x / std::numbers::sqrt2));
}

[[nodiscard]] double norm_pdf(double x) {
    const double sqrt2pi = std::sqrt(2.0 * std::numbers::pi);
    return std::exp(-0.5 * x * x) / sqrt2pi;
}

/// Black-Scholes European call price.
[[nodiscard]] double bs_call(double S, double K, double r, double sigma, double T) {
    if (T <= 0.0) {
        return std::max(S - K, 0.0);
    }
    const double sqrt_T = std::sqrt(T);
    const double d1     = (std::log(S / K) + (r + 0.5 * sigma * sigma) * T) /
                          (sigma * sqrt_T);
    const double d2     = d1 - sigma * sqrt_T;
    return S * norm_cdf(d1) - K * std::exp(-r * T) * norm_cdf(d2);
}

/// Vega of a European call — derivative wrt sigma.
[[nodiscard]] double bs_vega(double S, double K, double r, double sigma, double T) {
    if (T <= 0.0) {
        return 0.0;
    }
    const double sqrt_T = std::sqrt(T);
    const double d1     = (std::log(S / K) + (r + 0.5 * sigma * sigma) * T) /
                          (sigma * sqrt_T);
    return S * norm_pdf(d1) * sqrt_T;
}

// -----------------------------------------------------------------------------
// Root-finding methods
// -----------------------------------------------------------------------------

/// Signature for a univariate function.
using ScalarFunc = double(*)(double);

/// Wrapper for BS call minus market price — root at target sigma.
struct BsCallObjective {
    double S, K, r, T, target_price;

    [[nodiscard]] double operator()(double sigma) const {
        return bs_call(S, K, r, sigma, T) - target_price;
    }
};

/// Wrapper returning f and f' (vega) for Newton.
struct BsCallObjectiveFprime {
    double S, K, r, T, target_price;

    /// Returns (f(σ), f'(σ)).
    [[nodiscard]] std::pair<double, double> eval(double sigma) const {
        const double f  = bs_call(S, K, r, sigma, T) - target_price;
        const double fp = bs_vega(S, K, r, sigma, T);
        return {f, fp};
    }
};

/// Bisection method — guaranteed to converge if sign change exists.
[[nodiscard]] double bisection(const BsCallObjective& obj,
                                double a, double b,
                                double tol = 1e-8,
                                int max_iter = 200) {
    double fa = obj(a);
    double fb = obj(b);

    if (fa * fb >= 0.0) {
        throw std::runtime_error("bisection: no sign change in [a, b]");
    }

    double mid = 0.0;
    for (int i = 0; i < max_iter; ++i) {
        mid = 0.5 * (a + b);
        const double fm = obj(mid);

        if (std::abs(fm) < tol || 0.5 * (b - a) < tol) {
            return mid;
        }
        if (fa * fm < 0.0) {
            b = mid;
            fb = fm;
        } else {
            a = mid;
            fa = fm;
        }
    }
    return mid;
}

/// Newton–Raphson — quadratic convergence but needs good initial guess.
[[nodiscard]] double newton_raphson(const BsCallObjectiveFprime& obj,
                                     double x0,
                                     double tol = 1e-8,
                                     int max_iter = 100) {
    double x = x0;
    for (int i = 0; i < max_iter; ++i) {
        const auto [f, fp] = obj.eval(x);

        if (std::abs(f) < tol) {
            return x;
        }
        if (std::abs(fp) < 1e-15) {
            throw std::runtime_error("newton: zero derivative");
        }
        x = x - f / fp;
    }
    throw std::runtime_error("newton: did not converge");
}

}  // namespace quant

int main() {
    using namespace quant;

    std::cout << "=== Module 06: Root Finding & Implied Volatility ===\n\n";

    // Market inputs — European call trading at $6.50
    constexpr double S     = 100.00;
    constexpr double K     = 105.00;
    constexpr double r     = 0.03;
    constexpr double T     = 0.50;
    constexpr double mkt_price = 6.50;

    std::cout << std::format("Option: S={:.2f}, K={:.2f}, r={:.2f}%, T={:.1f}y\n",
                             S, K, r * 100.0, T);
    std::cout << std::format("Market price: {:.2f}\n\n", mkt_price);

    // Objective: f(σ) = BS(σ) - market price
    const BsCallObjective obj{S, K, r, T, mkt_price};

    // -------------------------------------------------------------------------
    // 1. Bisection
    // -------------------------------------------------------------------------
    {
        const double sigma_a = 0.05;
        const double sigma_b = 1.00;

        const double iv = bisection(obj, sigma_a, sigma_b);
        const double px = bs_call(S, K, r, iv, T);

        std::cout << "1. Bisection method\n";
        std::cout << std::format("   Implied vol: {:.6f} ({:.2f}%)\n", iv, iv * 100.0);
        std::cout << std::format("   Re-price    : {:.6f}\n", px);
        std::cout << std::format("   Error       : {:.2e}\n\n", std::abs(px - mkt_price));
    }

    // -------------------------------------------------------------------------
    // 2. Newton–Raphson
    // -------------------------------------------------------------------------
    {
        const BsCallObjectiveFprime obj_fp{S, K, r, T, mkt_price};
        constexpr double initial_guess = 0.30;

        try {
            const double iv = newton_raphson(obj_fp, initial_guess);
            const double px = bs_call(S, K, r, iv, T);

            std::cout << "2. Newton–Raphson\n";
            std::cout << std::format("   Implied vol: {:.6f} ({:.2f}%)\n", iv, iv * 100.0);
            std::cout << std::format("   Re-price    : {:.6f}\n", px);
            std::cout << std::format("   Error       : {:.2e}\n", std::abs(px - mkt_price));
            std::cout << std::format("   Iterations  : few (quadratic convergence)\n");
        } catch (const std::exception& e) {
            std::cout << "   Newton failed: " << e.what() << '\n';
        }
    }

    // -------------------------------------------------------------------------
    // 3. Volatility smile sketch — reprice at different vols
    // -------------------------------------------------------------------------
    std::cout << "\n3. Pricing at different volatilities (smile sketch):\n";
    for (double sigma = 0.10; sigma <= 0.70; sigma += 0.10) {
        const double price = bs_call(S, K, r, sigma, T);
        std::cout << std::format("   sigma = {:.0f}%  →  price = {:.4f}\n",
                                 sigma * 100.0, price);
    }

    std::cout << "\nRoot finding + Black-Scholes = implied vol engine.\n"
                 "Next: optimization and curve calibration.\n";

    return 0;
}
