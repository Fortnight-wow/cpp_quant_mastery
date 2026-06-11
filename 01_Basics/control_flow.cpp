/*
 * ============================================================================
 * MODULE 01 — LESSON 3: Control Flow for Trading Logic & Risk Rules
 * ============================================================================
 *
 * CONCEPT
 * -------
 * Control flow (`if`, `else`, `switch`, loops) encodes the decision rules
 * that govern trading systems:
 *
 *   - Risk limits: flatten if drawdown exceeds threshold
 *   - Order routing: market vs limit based on urgency and spread
 *   - Batch analytics: iterate over return series for volatility estimates
 *
 * This lesson covers:
 *   - Conditional execution for stop-loss / take-profit
 *   - `switch` for discrete order-type enumeration
 *   - `for` loops over daily returns (sample variance → volatility)
 *   - Range-based for (C++11) and `std::ranges` min/max (C++20)
 *
 * FINANCIAL EXAMPLE
 * -----------------
 * 1) A long position with stop-loss and take-profit triggers
 * 2) Order-type dispatch via enum class + switch
 * 3) Historical daily returns → sample standard deviation (annualized σ)
 *
 * Sample variance (Bessel corrected):
 *
 *   s² = (1 / (n - 1)) · Σ (rᵢ - r̄)²
 *
 * Annualized volatility (252 trading days):
 *
 *   σ_annual ≈ s · √252
 *
 * BUILD
 * -----
 *   cmake --build build --target control_flow
 *   ./build/01_Basics/control_flow
 * ============================================================================
 */

#include <algorithm>
#include <cmath>
#include <format>
#include <iostream>
#include <numeric>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>

namespace {

enum class OrderType { Market, Limit, Stop, StopLimit };

[[nodiscard]] std::string_view order_type_label(OrderType type) {
    switch (type) {
        case OrderType::Market:    return "MARKET";
        case OrderType::Limit:     return "LIMIT";
        case OrderType::Stop:      return "STOP";
        case OrderType::StopLimit: return "STOP_LIMIT";
    }
    return "UNKNOWN";
}

/// Evaluate stop-loss / take-profit; returns action string for demo output.
[[nodiscard]] std::string_view evaluate_risk_triggers(
    double entry_price,
    double current_price,
    double stop_loss_pct,
    double take_profit_pct) {

    const double pnl_pct = (current_price / entry_price - 1.0) * 100.0;

    if (pnl_pct <= -stop_loss_pct) {
        return "FLATTEN — stop-loss breached";
    }
    if (pnl_pct >= take_profit_pct) {
        return "REDUCE — take-profit target hit";
    }
    return "HOLD — within risk band";
}

/// Sample standard deviation of a return series; empty span returns 0.
[[nodiscard]] double sample_std_dev(std::span<const double> returns) {
    if (returns.size() < 2) {
        return 0.0;
    }

    const double mean = std::accumulate(returns.begin(), returns.end(), 0.0) /
                        static_cast<double>(returns.size());

    double sum_sq_dev = 0.0;
    for (const double r : returns) {
        const double dev = r - mean;
        sum_sq_dev += dev * dev;
    }

    return std::sqrt(sum_sq_dev / static_cast<double>(returns.size() - 1));
}

}  // namespace

int main() {
    // -------------------------------------------------------------------------
    // Part 1 — Stop-loss / take-profit decision tree
    // -------------------------------------------------------------------------
    constexpr double entry_price     = 100.00;
    constexpr double stop_loss_pct   = 5.0;   // exit if down 5%
    constexpr double take_profit_pct = 12.0;  // trim if up 12%

    const std::vector<double> mark_scenarios{94.50, 100.00, 112.50, 88.00};

    std::cout << "=== Risk Trigger Simulation (entry = $100.00) ===\n";
    for (const double mark : mark_scenarios) {
        const double pnl_pct = (mark / entry_price - 1.0) * 100.0;
        const auto   action  = evaluate_risk_triggers(
            entry_price, mark, stop_loss_pct, take_profit_pct);

        std::cout << std::format(
            "Mark ${:6.2f} | P&L {:+5.2f}% | {}\n",
            mark,
            pnl_pct,
            action);
    }

    // -------------------------------------------------------------------------
    // Part 2 — switch on enum class (order routing)
    // -------------------------------------------------------------------------
    std::cout << "\n=== Order Type Dispatch ===\n";
    const std::vector<OrderType> routed_orders{
        OrderType::Limit,
        OrderType::Market,
        OrderType::StopLimit,
    };

    for (const OrderType type : routed_orders) {
        std::cout << std::format(
            "Route order as {} → send to matching engine queue\n",
            order_type_label(type));
    }

    // -------------------------------------------------------------------------
    // Part 3 — Loop over daily returns → annualized volatility estimate
    // -------------------------------------------------------------------------
    // Ten hypothetical daily log-return fractions (not percent).
    const std::vector<double> daily_returns{
        0.012, -0.008, 0.004, 0.001, -0.015,
        0.009,  0.002, -0.003, 0.006, -0.001,
    };

    const double daily_vol   = sample_std_dev(daily_returns);
    constexpr double trading_days_per_year = 252.0;
    const double annual_vol  = daily_vol * std::sqrt(trading_days_per_year);

    const auto [min_ret, max_ret] = std::ranges::minmax_element(daily_returns);
    const double min_r = *min_ret;
    const double max_r = *max_ret;

    std::cout << "\n=== Realized Volatility from Daily Returns ===\n";
    std::cout << std::format("Observations (n)     : {}\n", daily_returns.size());
    std::cout << std::format("Min / Max daily ret  : {:+.3f} / {:+.3f}\n", min_r, max_r);
    std::cout << std::format("Daily std dev (s)    : {:.5f}\n", daily_vol);
    std::cout << std::format("Annualized sigma     : {:.2f}%  (s × sqrt(252))\n",
                             annual_vol * 100.0);

    std::cout << "\nControl flow mastered — next: memory, ownership, and RAII.\n";
    return 0;
}
