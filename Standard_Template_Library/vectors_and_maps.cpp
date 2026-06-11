/*
 * ============================================================================
 * MODULE 04 — LESSON 1: std::vector and Associative Maps in Market Data
 * ============================================================================
 *
 * CONCEPT
 * -------
 * The STL provides battle-tested containers used throughout trading stacks:
 *
 *   std::vector<T>              — Contiguous dynamic array; O(1) amortized
 *                                 push_back; ideal for time series, tick buffers,
 *                                 and Monte Carlo path storage.
 *
 *   std::map<K,V>               — Ordered red-black tree; O(log n) lookup;
 *                                 sorted iteration by key (e.g., yield curve pillars).
 *
 *   std::unordered_map<K,V>     — Hash table; O(1) average lookup; symbol →
 *                                 last price caches, instrument static data.
 *
 * Container choice drives latency and memory layout:
 *   - Scanning sequential ticks → vector (cache-friendly)
 *   - Random symbol lookup at high frequency → unordered_map
 *   - Ordered curve pillars for interpolation → map
 *
 * FINANCIAL EXAMPLE
 * -----------------
 * 1) Store an intraday mid-price series in a vector; compute session return.
 * 2) Maintain a symbol → last trade map for a watchlist.
 * 3) Build a sorted discount-factor curve (map: tenor → DF) for bond PV.
 *
 * Simple holding-period return from first to last mid:
 *
 *   R = (P_n / P_0) - 1
 *
 * BUILD
 * -----
 *   cmake --build build --target vectors_and_maps
 *   ./build/04_Standard_Template_Library/vectors_and_maps
 * ============================================================================
 */

#include <cmath>
#include <format>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

struct Quote {
    std::string symbol;
    double      last_price;
    double      bid;
    double      ask;
};

[[nodiscard]] double mid_price(const Quote& q) {
    return 0.5 * (q.bid + q.ask);
}

/// Linear interpolation on sorted map of tenor (years) → discount factor.
[[nodiscard]] double discount_factor(const std::map<double, double>& curve, double tenor) {
    if (curve.empty()) {
        return 1.0;
    }

    auto it = curve.lower_bound(tenor);
    if (it == curve.begin()) {
        return it->second;
    }
    if (it == curve.end()) {
        return curve.rbegin()->second;
    }

    const auto it_hi = it;
    const auto it_lo = std::prev(it);

    const double t0 = it_lo->first;
    const double t1 = it_hi->first;
    const double df0 = it_lo->second;
    const double df1 = it_hi->second;

    const double w = (tenor - t0) / (t1 - t0);
    return df0 + w * (df1 - df0);
}

}  // namespace

int main() {
    std::cout << "=== Module 04: Vectors and Maps ===\n\n";

    // -------------------------------------------------------------------------
    // 1. vector — intraday mid-price series (cache-friendly sequential access)
    // -------------------------------------------------------------------------
    std::vector<double> mid_series{
        174.10, 174.55, 175.00, 174.80, 175.20, 175.50,
    };

    const double open_mid  = mid_series.front();
    const double close_mid = mid_series.back();
    const double session_return = close_mid / open_mid - 1.0;

    std::cout << "Intraday mid series (vector)\n";
    std::cout << std::format("  Observations : {}\n", mid_series.size());
    std::cout << std::format("  Open mid     : {:.2f}\n", open_mid);
    std::cout << std::format("  Close mid    : {:.2f}\n", close_mid);
    std::cout << std::format("  Session ret  : {:+.3f}%\n\n", session_return * 100.0);

    // Reserve avoids reallocations when tick count is known (production pattern)
    std::vector<double> tick_buffer;
    tick_buffer.reserve(1'024);
    for (double px : mid_series) {
        tick_buffer.push_back(px);
    }
    std::cout << std::format("Reserved buffer size/capacity: {}/{}\n\n",
                             tick_buffer.size(),
                             tick_buffer.capacity());

    // -------------------------------------------------------------------------
    // 2. unordered_map — O(1) avg lookup: symbol → last price
    // -------------------------------------------------------------------------
    std::unordered_map<std::string, Quote> watchlist;
    watchlist.reserve(16);

    watchlist.emplace("AAPL", Quote{"AAPL", 175.50, 175.48, 175.52});
    watchlist.emplace("MSFT", Quote{"MSFT", 412.80, 412.75, 412.85});
    watchlist.emplace("GOOG", Quote{"GOOG", 171.20, 171.18, 171.22});

    constexpr std::string_view query_symbol = "MSFT";
    if (const auto it = watchlist.find(std::string{query_symbol}); it != watchlist.end()) {
        const Quote& q = it->second;
        std::cout << std::format("Watchlist lookup '{}'\n", query_symbol);
        std::cout << std::format("  Last : {:.2f}  Mid : {:.2f}\n\n",
                                 q.last_price,
                                 mid_price(q));
    }

    // -------------------------------------------------------------------------
    // 3. map — sorted tenor → discount factor curve (pillar interpolation)
    // -------------------------------------------------------------------------
    std::map<double, double> discount_curve{
        {0.25, std::exp(-0.043 * 0.25)},
        {0.50, std::exp(-0.044 * 0.50)},
        {1.00, std::exp(-0.045 * 1.00)},
        {2.00, std::exp(-0.046 * 2.00)},
    };

    constexpr double face_value   = 100'000.0;
    constexpr double coupon_rate  = 0.05;
    constexpr double coupon_tenor = 0.75;  // years — between pillars

    const double df_coupon = discount_factor(discount_curve, coupon_tenor);
    const double pv_coupon = face_value * coupon_rate * df_coupon;

    std::cout << "Discount curve (std::map — sorted by tenor)\n";
    for (const auto& [tenor, df] : discount_curve) {
        std::cout << std::format("  T={:.2f}y  DF={:.6f}\n", tenor, df);
    }

    std::cout << std::format("\nInterpolated DF at T={:.2f}y : {:.6f}\n",
                             coupon_tenor,
                             df_coupon);
    std::cout << std::format("PV of single coupon ({:.0f}% on ${:.0f}) : ${:.2f}\n",
                             coupon_rate * 100.0,
                             face_value,
                             pv_coupon);

    std::cout << "\nRule of thumb: vector for series, unordered_map for fast keys,\n"
                 "map when sorted order matters.\n";

    return 0;
}
