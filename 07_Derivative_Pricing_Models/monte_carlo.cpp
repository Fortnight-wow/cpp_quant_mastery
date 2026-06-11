/*
 * ============================================================================
 * MODULE 07 — LESSON 3: Monte Carlo Simulation & Path-Dependent Options
 * ============================================================================
 *
 * CONCEPT
 * -------
 * Monte Carlo simulates many independent paths of the underlying under the
 * risk-neutral measure and averages the discounted payoff:
 *
 *   V = e^{-rT} · E[ payoff(S_T) ]
 *
 * Risk-neutral GBM discretisation (Euler–Maruyama, exact for log-Euler):
 *
 *   S_T = S_0 · exp((r - ½σ²)T + σ√T·Z)    where Z ~ N(0, 1)
 *
 * Variance reduction:
 *   - Antithetic variates: for each Z, also run with -Z
 *   - This halves the number of independent samples needed for same precision
 *
 * FINANCIAL EXAMPLE
 * -----------------
 * 1) Price a European call via plain MC (with convergence demonstration).
 * 2) Price same call with antithetic variates — show variance reduction.
 * 3) Price an Asian (arithmetic average) call — path-dependent payoff.
 *
 * Asian call payoff:
 *   payoff = max(avg(S) - K, 0)
 *   avg(S) = (1/N) Σ_{i=1}^{N} S(t_i)
 *
 * BUILD
 * -----
 *   cmake --build build --target monte_carlo
 *   ./build/07_Derivative_Pricing_Models/monte_carlo
 * ============================================================================
 */

#include <algorithm>
#include <cmath>
#include <format>
#include <iostream>
#include <numbers>
#include <random>
#include <vector>

namespace quant {

/// Generate standard normal random variates via Box-Muller.
[[nodiscard]] std::vector<double> gaussian_sequence(std::size_t n) {
    static std::mt19937_64 rng{std::random_device{}()};
    static std::normal_distribution<double> dist{0.0, 1.0};

    std::vector<double> z(n);
    for (auto& zi : z) {
        zi = dist(rng);
    }
    return z;
}

/// Black-Scholes analytical price for comparison.
[[nodiscard]] double bs_call_price(double S, double K, double r,
                                    double sigma, double T) {
    if (T <= 0.0) return std::max(S - K, 0.0);
    const double sqrt_T = std::sqrt(T);
    const double d1 = (std::log(S / K) + (r + 0.5 * sigma * sigma) * T) /
                      (sigma * sqrt_T);
    const double d2 = d1 - sigma * sqrt_T;
    const auto norm_cdf = [](double x) {
        return 0.5 * (1.0 + std::erf(x / std::numbers::sqrt2));
    };
    return S * norm_cdf(d1) - K * std::exp(-r * T) * norm_cdf(d2);
}

/// Plain Monte Carlo for European call.
struct McResult {
    double price;
    double std_error;
};

[[nodiscard]] McResult mc_european_call(
    double S, double K, double r, double sigma, double T,
    std::uint64_t num_paths) {

    const double drift  = (r - 0.5 * sigma * sigma) * T;
    const double diffusion = sigma * std::sqrt(T);
    const double df     = std::exp(-r * T);

    const auto z = gaussian_sequence(num_paths);

    double sum_payoff = 0.0;
    double sum_sq     = 0.0;

    for (std::uint64_t i = 0; i < num_paths; ++i) {
        const double ST = S * std::exp(drift + diffusion * z[i]);
        const double payoff = std::max(ST - K, 0.0);
        sum_payoff += payoff;
        sum_sq     += payoff * payoff;
    }

    const double mean = sum_payoff / static_cast<double>(num_paths);
    const double var  = (sum_sq / static_cast<double>(num_paths) - mean * mean) /
                        static_cast<double>(num_paths);

    return McResult{
        .price     = df * mean,
        .std_error = df * std::sqrt(var),
    };
}

/// Antithetic Monte Carlo — pairs (Z, -Z) for variance reduction.
[[nodiscard]] McResult mc_antithetic_call(
    double S, double K, double r, double sigma, double T,
    std::uint64_t num_pairs) {

    const double drift  = (r - 0.5 * sigma * sigma) * T;
    const double diffusion = sigma * std::sqrt(T);
    const double df     = std::exp(-r * T);

    const auto z = gaussian_sequence(num_pairs);

    double sum_payoff = 0.0;
    double sum_sq     = 0.0;

    for (std::uint64_t i = 0; i < num_pairs; ++i) {
        const double ST1 = S * std::exp(drift + diffusion * z[i]);
        const double ST2 = S * std::exp(drift - diffusion * z[i]);
        const double payoff = 0.5 * (std::max(ST1 - K, 0.0) + std::max(ST2 - K, 0.0));
        sum_payoff += payoff;
        sum_sq     += payoff * payoff;
    }

    const double mean = sum_payoff / static_cast<double>(num_pairs);
    const double var  = (sum_sq / static_cast<double>(num_pairs) - mean * mean) /
                        static_cast<double>(num_pairs);

    return McResult{
        .price     = df * mean,
        .std_error = df * std::sqrt(var),
    };
}

/// Monte Carlo for Asian (arithmetic average) call.
[[nodiscard]] McResult mc_asian_call(
    double S, double K, double r, double sigma, double T,
    std::uint64_t num_paths, int num_monitoring_dates) {

    const double dt      = T / static_cast<double>(num_monitoring_dates);
    const double drift   = (r - 0.5 * sigma * sigma) * dt;
    const double diff    = sigma * std::sqrt(dt);
    const double df      = std::exp(-r * T);

    std::mt19937_64 rng{std::random_device{}()};
    std::normal_distribution<double> dist{0.0, 1.0};

    double sum_payoff = 0.0;
    double sum_sq     = 0.0;

    for (std::uint64_t path = 0; path < num_paths; ++path) {
        double St = S;
        double sum_St = S;  // include S_0 in average

        for (int t = 0; t < num_monitoring_dates; ++t) {
            St *= std::exp(drift + diff * dist(rng));
            sum_St += St;
        }

        const double avg = sum_St / static_cast<double>(num_monitoring_dates + 1);
        const double payoff = std::max(avg - K, 0.0);

        sum_payoff += payoff;
        sum_sq     += payoff * payoff;
    }

    const double mean = sum_payoff / static_cast<double>(num_paths);
    const double var  = (sum_sq / static_cast<double>(num_paths) - mean * mean) /
                        static_cast<double>(num_paths);

    return McResult{
        .price     = df * mean,
        .std_error = df * std::sqrt(var),
    };
}

}  // namespace quant

int main() {
    using namespace quant;

    std::cout << "=== Module 07: Monte Carlo Simulation ===\n\n";

    constexpr double S     = 175.50;
    constexpr double K     = 180.00;
    constexpr double r     = 0.045;
    constexpr double sigma = 0.22;
    constexpr double T     = 0.25;

    std::cout << std::format("Parameters: S={:.2f}, K={:.2f}, r={:.2f}%, "
                             "σ={:.0f}%, T={:.4f}y\n\n",
                             S, K, r * 100.0, sigma * 100.0, T);

    // -------------------------------------------------------------------------
    // 1. Plain MC European call — convergence with paths
    // -------------------------------------------------------------------------
    std::cout << "1. Plain Monte Carlo — European Call\n";
    const double bs = bs_call_price(S, K, r, sigma, T);
    std::cout << std::format("   Black-Scholes (exact) : {:.6f}\n\n", bs);

    for (std::uint64_t paths : {10'000, 100'000, 1'000'000}) {
        const auto mc = mc_european_call(S, K, r, sigma, T, paths);
        std::cout << std::format("   {:>8} paths: price={:.6f}, se={:.6f},"
                                 " err={:.2e}\n",
                                 paths, mc.price, mc.std_error,
                                 std::abs(mc.price - bs));
    }

    // -------------------------------------------------------------------------
    // 2. Antithetic variates — variance reduction
    // -------------------------------------------------------------------------
    std::cout << "\n2. Antithetic Variates (variance reduction)\n";
    for (std::uint64_t pairs : {5'000, 50'000, 500'000}) {
        const auto mc = mc_antithetic_call(S, K, r, sigma, T, pairs);
        std::cout << std::format("   {:>8} pairs: price={:.6f}, se={:.6f},"
                                 " err={:.2e}\n",
                                 pairs, mc.price, mc.std_error,
                                 std::abs(mc.price - bs));
    }

    // -------------------------------------------------------------------------
    // 3. Asian (arithmetic average) call — path-dependent
    // -------------------------------------------------------------------------
    std::cout << "\n3. Asian Call (arithmetic average, 12 monthly observations)\n";
    const auto asian = mc_asian_call(S, K, r, sigma, T, 200'000, 12);
    std::cout << std::format("   Price = {:.6f}  (se = {:.6f})\n",
                             asian.price, asian.std_error);

    // Asian options are cheaper than vanilla (averaging reduces vol)
    std::cout << std::format("   BS vanilla call = {:.6f} vs Asian = {:.6f}"
                             " — averaging reduces vol\n\n",
                             bs, asian.price);

    std::cout << "--- Summary ---\n";
    std::cout << "  MC converges at O(1/√N). Antithetic halves the constant.\n";
    std::cout << "  Asian options demonstrate path-dependent payoff handling.\n";
    std::cout << "  Module 07 complete — ready for trading systems.\n";

    return 0;
}
