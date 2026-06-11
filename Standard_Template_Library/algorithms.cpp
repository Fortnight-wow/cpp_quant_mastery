/*
 * ============================================================================
 * MODULE 04 — LESSON 2: STL Algorithms and C++20 Ranges
 * ============================================================================
 *
 * CONCEPT
 * -------
 * `<algorithm>` and `<numeric>` provide generic, optimized operations over
 * iterators (and C++20 ranges). Instead of hand-written loops, quant code uses:
 *
 *   std::sort          — order trade blotter by notional
 *   std::accumulate    — sum P&L legs, cash flows
 *   std::lower_bound   — locate pillar on sorted curve
 *   std::ranges::sort  — range-based variant (cleaner syntax)
 *   views::filter/transform — lazy pipelines for analytics
 *
 * Complexity matters in production:
 *   sort: O(n log n)   accumulate: O(n)   lower_bound: O(log n)
 *
 * FINANCIAL EXAMPLE
 * -----------------
 * 1) Sort desk trades by absolute notional (risk ranking).
 * 2) Accumulate daily P&L vector → book total.
 * 3) Filter + transform return series → compute mean return via accumulate.
 * 4) lower_bound on sorted strikes to find bracket for interpolation.
 *
 * Sample mean return:
 *
 *   r̄ = (1/n) Σ rᵢ
 *
 * BUILD
 * -----
 *   cmake --build build --target algorithms
 *   ./build/04_Standard_Template_Library/algorithms
 * ============================================================================
 */

#include <algorithm>
#include <cmath>
#include <format>
#include <iostream>
#include <iterator>
#include <numeric>
#include <ranges>
#include <string>
#include <vector>

namespace {

struct Trade {
    std::string symbol;
    double      quantity;
    double      price;
    double      pnl;

    [[nodiscard]] double notional() const {
        return std::abs(quantity * price);
    }
};

[[nodiscard]] double mean_return(const std::vector<double>& returns) {
    if (returns.empty()) {
        return 0.0;
    }
    const double sum = std::accumulate(returns.begin(), returns.end(), 0.0);
    return sum / static_cast<double>(returns.size());
}

}  // namespace

int main() {
    std::cout << "=== Module 04: Algorithms & Ranges ===\n\n";

    // -------------------------------------------------------------------------
    // 1. std::ranges::sort — rank trades by notional (largest risk first)
    // -------------------------------------------------------------------------
    std::vector<Trade> blotter{
        {"AAPL",  1'000, 175.50,  2'500.0},
        {"TSLA",   -200, 248.00, -1'200.0},
        {"MSFT",   2'500, 412.80,  8'000.0},
        {"NVDA",    150, 875.00,  4'500.0},
    };

    std::ranges::sort(blotter, {}, &Trade::notional);
    std::ranges::reverse(blotter);  // descending notional

    std::cout << "Blotter sorted by |notional| (desc)\n";
    std::cout << std::format("{:<6} {:>10} {:>10} {:>12}\n",
                             "Symbol", "Qty", "Price", "|Notional|");
    std::cout << std::string(42, '-') << '\n';

    for (const Trade& t : blotter) {
        std::cout << std::format("{:<6} {:>10.0f} {:>10.2f} {:>12.2f}\n",
                                 t.symbol,
                                 t.quantity,
                                 t.price,
                                 t.notional());
    }

    // -------------------------------------------------------------------------
    // 2. accumulate — total book P&L
    // -------------------------------------------------------------------------
    const double total_pnl = std::accumulate(
        blotter.begin(),
        blotter.end(),
        0.0,
        [](double acc, const Trade& t) { return acc + t.pnl; });

    std::cout << std::format("\nTotal desk P&L (accumulate) : ${:+.2f}\n\n", total_pnl);

    // -------------------------------------------------------------------------
    // 3. ranges views — filter large |returns|, scale to bps, then average
    // -------------------------------------------------------------------------
    const std::vector<double> daily_returns{
        0.008, -0.003, 0.001, -0.012, 0.004, 0.000, 0.006, -0.001,
    };

    auto large_moves = daily_returns
        | std::views::filter([](double r) { return std::abs(r) >= 0.004; })
        | std::views::transform([](double r) { return r * 10'000.0; });  // to bps

    std::vector<double> large_move_bps;
    large_move_bps.reserve(daily_returns.size());
    for (double bps : large_moves) {
        large_move_bps.push_back(bps);
    }

    const double avg_large_bps = large_move_bps.empty()
        ? 0.0
        : std::accumulate(large_move_bps.begin(), large_move_bps.end(), 0.0)
              / static_cast<double>(large_move_bps.size());

    std::cout << "Daily returns — filter |r| >= 40 bps, express in bps\n";
    for (double bps : large_move_bps) {
        std::cout << std::format("  {:+.1f} bps\n", bps);
    }
    std::cout << std::format("  Average of filtered moves: {:+.2f} bps\n\n", avg_large_bps);

    const double overall_mean = mean_return(daily_returns);
    std::cout << std::format("Mean daily return (all {} days): {:+.4f} ({:+.2f} bps)\n\n",
                             daily_returns.size(),
                             overall_mean,
                             overall_mean * 10'000.0);

    // -------------------------------------------------------------------------
    // 4. lower_bound — strike bracket on sorted option chain
    // -------------------------------------------------------------------------
    std::vector<double> strikes{160.0, 165.0, 170.0, 175.0, 180.0, 185.0, 190.0};
    std::ranges::sort(strikes);

    constexpr double target_spot = 177.25;
    const auto lb = std::lower_bound(strikes.begin(), strikes.end(), target_spot);

    const double strike_hi = (lb != strikes.end()) ? *lb : strikes.back();
    const double strike_lo = (lb != strikes.begin()) ? *std::prev(lb) : strikes.front();

    std::cout << std::format("Sorted strikes — bracket spot {:.2f}\n", target_spot);
    std::cout << std::format("  Lower strike : {:.2f}\n", strike_lo);
    std::cout << std::format("  Upper strike : {:.2f}\n", strike_hi);

    std::cout << "\nPrefer STL algorithms over raw loops: fewer bugs, known complexity.\n";

    return 0;
}
