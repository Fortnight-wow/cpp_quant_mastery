/*
 * ============================================================================
 * MODULE 07 — LESSON 2: Binomial Tree (Cox–Ross–Rubinstein)
 * ============================================================================
 *
 * CONCEPT
 * -------
 * The binomial tree discretises the GBM process into a recombining lattice:
 *
 *   S_{i+1} = S_i × u   (up move, prob p)
 *   S_{i+1} = S_i × d   (down move, prob 1-p)
 *
 * Cox–Ross–Rubinstein parameters:
 *   u = exp(σ·√Δt)
 *   d = 1/u
 *   p = (exp(r·Δt) - d) / (u - d)
 *
 * Pricing proceeds backwards:
 *   V_{i,j} = e^{-r·Δt} · [p · V_{i+1,j+1} + (1-p) · V_{i+1,j}]
 *
 * For American options, we check early exercise at each node:
 *   V_{i,j} = max(intrinsic, continuation)
 *
 * FINANCIAL EXAMPLE
 * -----------------
 * Price a 3-month ATM European call and an American put, comparing with
 * Black-Scholes for the European case. Demonstrate tree convergence as
 * steps increase.
 *
 * BUILD
 * -----
 *   cmake --build build --target binomial_tree
 *   ./build/07_Derivative_Pricing_Models/binomial_tree
 * ============================================================================
 */

#include <algorithm>
#include <cmath>
#include <format>
#include <iostream>
#include <numbers>
#include <vector>

namespace quant {

/// Price a European / American option via CRR binomial tree.
[[nodiscard]] double binomial_price(
    double S, double K, double r, double sigma, double T,
    int steps, bool is_call, bool is_american) {

    const double dt = T / static_cast<double>(steps);
    const double u  = std::exp(sigma * std::sqrt(dt));
    const double d  = 1.0 / u;
    const double p  = (std::exp(r * dt) - d) / (u - d);
    const double df = std::exp(-r * dt);

    // Asset prices at terminal nodes
    std::vector<double> prices(steps + 1);
    for (int j = 0; j <= steps; ++j) {
        prices[j] = S * std::pow(u, static_cast<double>(j)) *
                    std::pow(d, static_cast<double>(steps - j));
    }

    // Terminal payoffs
    std::vector<double> values(steps + 1);
    for (int j = 0; j <= steps; ++j) {
        if (is_call) {
            values[j] = std::max(prices[j] - K, 0.0);
        } else {
            values[j] = std::max(K - prices[j], 0.0);
        }
    }

    // Backward induction
    for (int i = steps - 1; i >= 0; --i) {
        // Precompute the stock price at the start of this layer's first node
        // (only needed for American exercise check)
        for (int j = 0; j <= i; ++j) {
            double continuation = df * (p * values[j + 1] + (1.0 - p) * values[j]);

            if (is_american) {
                const double node_S = S * std::pow(u, static_cast<double>(j)) *
                                      std::pow(d, static_cast<double>(i - j));
                double intrinsic;
                if (is_call) {
                    intrinsic = std::max(node_S - K, 0.0);
                } else {
                    intrinsic = std::max(K - node_S, 0.0);
                }
                values[j] = std::max(continuation, intrinsic);
            } else {
                values[j] = continuation;
            }
        }
    }

    return values[0];
}

/// Black-Scholes price for comparison (European only).
[[nodiscard]] double bs_price(double S, double K, double r,
                               double sigma, double T, bool is_call) {
    if (T <= 0.0) {
        return is_call ? std::max(S - K, 0.0) : std::max(K - S, 0.0);
    }
    const double sqrt_T = std::sqrt(T);
    const double d1     = (std::log(S / K) + (r + 0.5 * sigma * sigma) * T) /
                          (sigma * sqrt_T);
    const double d2     = d1 - sigma * sqrt_T;

    const auto norm_cdf = [](double x) {
        return 0.5 * (1.0 + std::erf(x / std::numbers::sqrt2));
    };

    if (is_call) {
        return S * norm_cdf(d1) - K * std::exp(-r * T) * norm_cdf(d2);
    }
    return K * std::exp(-r * T) * norm_cdf(-d2) - S * norm_cdf(-d1);
}

}  // namespace quant

int main() {
    using namespace quant;

    std::cout << "=== Module 07: Binomial Tree (CRR) ===\n\n";

    constexpr double S     = 175.50;
    constexpr double K     = 180.00;
    constexpr double r     = 0.045;
    constexpr double sigma = 0.22;
    constexpr double T     = 0.25;

    std::cout << std::format("Parameters: S={:.2f}, K={:.2f}, r={:.2f}%, "
                             "σ={:.0f}%, T={:.4f}y\n\n",
                             S, K, r * 100.0, sigma * 100.0, T);

    // -------------------------------------------------------------------------
    // 1. European call — tree vs BS
    // -------------------------------------------------------------------------
    std::cout << "--- European Call ---\n";
    const double bs_call = bs_price(S, K, r, sigma, T, true);
    std::cout << std::format("  Black-Scholes    : {:.6f}\n", bs_call);

    for (int steps : {10, 50, 100, 200, 500}) {
        const double tree = binomial_price(S, K, r, sigma, T, steps, true, false);
        std::cout << std::format("  CRR Tree (n={:>3}) : {:.6f}  (err: {:.2e})\n",
                                 steps, tree, std::abs(tree - bs_call));
    }

    // -------------------------------------------------------------------------
    // 2. European put — tree vs BS
    // -------------------------------------------------------------------------
    std::cout << "\n--- European Put ---\n";
    const double bs_put = bs_price(S, K, r, sigma, T, false);
    std::cout << std::format("  Black-Scholes    : {:.6f}\n", bs_put);

    for (int steps : {10, 50, 100, 200}) {
        const double tree = binomial_price(S, K, r, sigma, T, steps, false, false);
        std::cout << std::format("  CRR Tree (n={:>3}) : {:.6f}  (err: {:.2e})\n",
                                 steps, tree, std::abs(tree - bs_put));
    }

    // -------------------------------------------------------------------------
    // 3. American put — early exercise premium
    // -------------------------------------------------------------------------
    std::cout << "\n--- American Put (early exercise) ---\n";
    for (int steps : {50, 100, 200, 500}) {
        const double euro = binomial_price(S, K, r, sigma, T, steps, false, false);
        const double amer = binomial_price(S, K, r, sigma, T, steps, false, true);
        std::cout << std::format("  n={:>3}  European: {:.6f}  American: {:.6f}"
                                 "  premium: {:.6f}\n",
                                 steps, euro, amer, amer - euro);
    }

    std::cout << "\nBinomial trees converge to BS for European options and\n"
                 "capture early exercise premium for American options.\n"
                 "Next: Monte Carlo simulation for path-dependent payoffs.\n";

    return 0;
}
