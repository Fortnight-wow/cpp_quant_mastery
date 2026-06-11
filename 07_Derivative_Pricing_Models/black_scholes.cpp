/*
 * ============================================================================
 * MODULE 07 — LESSON 1: Black–Scholes–Merton Closed-Form Pricing
 * ============================================================================
 *
 * CONCEPT
 * -------
 * The Black–Scholes–Merton PDE provides closed-form prices for European
 * vanilla options under geometric Brownian motion:
 *
 *   ∂V/∂t + ½σ²S² ∂²V/∂S² + rS ∂V/∂S - rV = 0
 *
 * European call:
 *   C = S·N(d₁) - K·e^{-rT}·N(d₂)
 *
 * European put:
 *   P = K·e^{-rT}·N(-d₂) - S·N(-d₁)
 *
 * where
 *   d₁ = [ln(S/K) + (r + σ²/2)T] / (σ√T)
 *   d₂ = d₁ - σ√T
 *
 * Greeks:
 *   Δ_call = N(d₁)       Γ = N'(d₁) / (S·σ·√T)
 *   ν (Vega) = S·N'(d₁)·√T
 *   Θ (Theta, call) = -S·N'(d₁)·σ / (2√T) - r·K·e^{-rT}·N(d₂)
 *   ρ (Rho, call)   = K·T·e^{-rT}·N(d₂)
 *
 * FINANCIAL EXAMPLE
 * -----------------
 * Price and risk a 3-month AAPL call option, compute all five Greeks,
 * and verify put-call parity.
 *
 * BUILD
 * -----
 *   cmake --build build --target black_scholes
 *   ./build/07_Derivative_Pricing_Models/black_scholes
 * ============================================================================
 */

#include <cmath>
#include <format>
#include <iostream>
#include <numbers>

namespace quant {

/// Standard normal CDF via std::erf.
[[nodiscard]] double norm_cdf(double x) {
    return 0.5 * (1.0 + std::erf(x / std::numbers::sqrt2));
}

/// Standard normal PDF.
[[nodiscard]] double norm_pdf(double x) {
    const double sqrt2pi = std::sqrt(2.0 * std::numbers::pi);
    return std::exp(-0.5 * x * x) / sqrt2pi;
}

struct BsResult {
    double price;
    double delta;
    double gamma;
    double vega;
    double theta;  // per calendar day
    double rho;
};

/// Compute Black-Scholes price and all Greeks for a European option.
[[nodiscard]] BsResult black_scholes(double S, double K, double r,
                                      double sigma, double T, bool is_call) {
    if (T <= 0.0) {
        // At expiry — intrinsic value only
        const double px = is_call ? std::max(S - K, 0.0) : std::max(K - S, 0.0);
        return BsResult{
            .price = px,
            .delta = is_call ? (S > K ? 1.0 : 0.0) : (S < K ? -1.0 : 0.0),
            .gamma = 0.0,
            .vega  = 0.0,
            .theta = 0.0,
            .rho   = 0.0,
        };
    }

    const double sqrt_T = std::sqrt(T);
    const double d1     = (std::log(S / K) + (r + 0.5 * sigma * sigma) * T) /
                          (sigma * sqrt_T);
    const double d2     = d1 - sigma * sqrt_T;

    const double nd1   = norm_cdf(d1);
    const double nd2   = norm_cdf(d2);
    const double npd1  = norm_pdf(d1);
    const double df    = std::exp(-r * T);  // discount factor

    double price;
    double delta;
    double gamma;
    double vega;
    double theta;
    double rho;

    if (is_call) {
        price = S * nd1 - K * df * nd2;
        delta = nd1;
        gamma = npd1 / (S * sigma * sqrt_T);
        vega  = S * npd1 * sqrt_T;
        // Theta per year (divide by 365 for per-day)
        theta = -S * npd1 * sigma / (2.0 * sqrt_T) - r * K * df * nd2;
        rho   = K * T * df * nd2;
    } else {
        const double n_neg_d1 = norm_cdf(-d1);
        const double n_neg_d2 = norm_cdf(-d2);
        price = K * df * n_neg_d2 - S * n_neg_d1;
        delta = nd1 - 1.0;
        gamma = npd1 / (S * sigma * sqrt_T);
        vega  = S * npd1 * sqrt_T;
        theta = -S * npd1 * sigma / (2.0 * sqrt_T) + r * K * df * n_neg_d2;
        rho   = -K * T * df * n_neg_d2;
    }

    return BsResult{
        .price = price,
        .delta = delta,
        .gamma = gamma,
        .vega  = vega,
        .theta = theta / 365.0,  // per calendar day
        .rho   = rho,
    };
}

/// Put-call parity: C - P = S - K·e^{-rT}
[[nodiscard]] bool check_put_call_parity(double call_price, double put_price,
                                          double S, double K, double r, double T,
                                          double tol = 1e-10) {
    const double lhs = call_price - put_price;
    const double rhs = S - K * std::exp(-r * T);
    return std::abs(lhs - rhs) < tol;
}

}  // namespace quant

int main() {
    using namespace quant;

    std::cout << "=== Module 07: Black-Scholes Pricing & Greeks ===\n\n";

    constexpr double S     = 175.50;
    constexpr double K     = 180.00;
    constexpr double r     = 0.045;
    constexpr double sigma = 0.22;
    constexpr double T     = 0.25;

    std::cout << std::format("Underlying: S={:.2f}, K={:.2f}, r={:.2f}%\n",
                             S, K, r * 100.0);
    std::cout << std::format("Vol: {:.0f}%,  T: {:.4f}y (~{} days)\n\n",
                             sigma * 100.0, T, static_cast<int>(T * 365.0));

    // Call
    const BsResult call = black_scholes(S, K, r, sigma, T, true);
    std::cout << "=== European CALL ===\n";
    std::cout << std::format("  Price : {:.6f}\n", call.price);
    std::cout << std::format("  Delta : {:.6f}\n", call.delta);
    std::cout << std::format("  Gamma : {:.6f}\n", call.gamma);
    std::cout << std::format("  Vega  : {:.6f}\n", call.vega);
    std::cout << std::format("  Theta : {:.6f} / day\n", call.theta);
    std::cout << std::format("  Rho   : {:.6f}\n\n", call.rho);

    // Put
    const BsResult put = black_scholes(S, K, r, sigma, T, false);
    std::cout << "=== European PUT ===\n";
    std::cout << std::format("  Price : {:.6f}\n", put.price);
    std::cout << std::format("  Delta : {:.6f}\n", put.delta);
    std::cout << std::format("  Gamma : {:.6f}\n", put.gamma);
    std::cout << std::format("  Vega  : {:.6f}\n", put.vega);
    std::cout << std::format("  Theta : {:.6f} / day\n", put.theta);
    std::cout << std::format("  Rho   : {:.6f}\n\n", put.rho);

    // Put-Call parity
    const bool parity_holds = check_put_call_parity(
        call.price, put.price, S, K, r, T);
    std::cout << std::format("Put-Call parity holds: {}\n",
                             parity_holds ? "YES ✓" : "NO ✗");

    const double parity_diff = (call.price - put.price) - (S - K * std::exp(-r * T));
    std::cout << std::format("  C - P - (S - K·e^-rT) = {:.2e}\n\n", parity_diff);

    // Greeks interpretation
    std::cout << "--- Greek interpretation ---\n";
    std::cout << std::format("  A 1$ spot ↑ → call price ↑ by {:.4f} (delta)\n", call.delta);
    std::cout << std::format("  A 1% vol ↑  → call price ↑ by {:.4f} (vega/100)\n", call.vega / 100.0);
    std::cout << std::format("  1 day decay → call price ↓ by {:.6f} (theta)\n", call.theta);

    std::cout << "\nClosed-form Greeks enable real-time risk computation.\n"
                 "Next: binomial trees for early exercise.\n";

    return 0;
}
