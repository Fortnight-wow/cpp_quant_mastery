/*
 * ============================================================================
 * MODULE 01 — LESSON 2: Fundamental Data Types in Quantitative Code
 * ============================================================================
 *
 * CONCEPT
 * -------
 * Financial software mixes exact integers (share counts, order quantities) with
 * approximate floating-point values (prices, yields, Greeks). Choosing the
 * wrong type causes subtle bugs:
 *
 *   - Use integers for discrete counts (never store 100 shares as 99.999…)
 *   - Use `double` (64-bit IEEE 754) for most pricing and risk calculations
 *   - Avoid `float` in production pricing paths (insufficient precision)
 *
 * IEEE 754 double provides ~15–16 decimal digits of precision. For a stock at
 * $175.50, that is more than adequate for notionals; for cumulative MC paths
 * or ill-conditioned linear algebra, precision limits matter (Module 06).
 *
 * KEY TYPES IN THIS LESSON
 * ------------------------
 *   bool          — flags (is_call, is_expired, trading_halted)
 *   int / int64_t — lot sizes, timestamps (epoch ms), bar counts
 *   double        — spot, strike, rate, vol, P&L, Greeks
 *   char          — occasional single-character venue codes
 *   std::string   — symbols, ISINs, human-readable labels
 *
 * FINANCIAL EXAMPLE
 * -----------------
 * We compute the cash market value and unrealized P&L of a long equity
 * position, demonstrating integer quantity × double price and the perils of
 * naive float accumulation on a long ledger.
 *
 * BUILD
 * -----
 *   cmake --build build --target data_types
 *   ./build/01_Basics/data_types
 * ============================================================================
 */

#include <cmath>
#include <cstdint>
#include <format>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>

int main() {
    // -------------------------------------------------------------------------
    // Position specification — designated initializers (C++20)
    // -------------------------------------------------------------------------
    struct Position {
        std::string_view symbol;
        std::int64_t     quantity_shares;  // exact integer — no fractional shares
        double           avg_entry_price;
        double           current_price;
        bool             is_long{true};
    };

    const Position book = {
        .symbol            = "MSFT",
        .quantity_shares   = 1'500,       // C++14 digit separator for readability
        .avg_entry_price   = 380.25,
        .current_price     = 412.80,
        .is_long           = true,
    };

    // -------------------------------------------------------------------------
    // Core P&L arithmetic — always double for monetary amounts
    // -------------------------------------------------------------------------
    const double notional      = static_cast<double>(book.quantity_shares) * book.current_price;
    const double cost_basis    = static_cast<double>(book.quantity_shares) * book.avg_entry_price;
    const double unrealized_pnl = notional - cost_basis;
    const double return_pct    = (book.current_price / book.avg_entry_price - 1.0) * 100.0;

    std::cout << std::format("=== Position: {} ===\n", book.symbol);
    std::cout << std::format("Side          : {}\n", book.is_long ? "LONG" : "SHORT");
    std::cout << std::format("Quantity      : {} shares (int64)\n", book.quantity_shares);
    std::cout << std::format("Entry price   : ${:.2f}\n", book.avg_entry_price);
    std::cout << std::format("Mark price    : ${:.2f}\n", book.current_price);
    std::cout << std::format("Notional      : ${:.2f}\n", notional);
    std::cout << std::format("Cost basis    : ${:.2f}\n", cost_basis);
    std::cout << std::format("Unrealized P&L: ${:+.2f}\n", unrealized_pnl);
    std::cout << std::format("Return        : {:+.2f}%\n\n", return_pct);

    // -------------------------------------------------------------------------
    // Precision demo: float vs double on repeated small additions
    // -------------------------------------------------------------------------
    // Simulates summing many tiny fee debits — a classic float drift scenario.
    constexpr int    num_legs = 10'000;
    constexpr double fee_per_leg_double = 0.01;

    double ledger_double = 0.0;
    float  ledger_float  = 0.0F;

    for (int i = 0; i < num_legs; ++i) {
        ledger_double += fee_per_leg_double;
        ledger_float  += static_cast<float>(fee_per_leg_double);
    }

    const double expected_fees = num_legs * fee_per_leg_double;

    std::cout << std::format(
        "=== Floating-Point Precision ({} fee legs × $0.01) ===\n", num_legs);
    std::cout << std::format("Expected total     : ${:.2f}\n", expected_fees);
    std::cout << std::format("double accumulator : ${:.6f}  (error: {:.2e})\n",
                             ledger_double,
                             std::abs(ledger_double - expected_fees));
    std::cout << std::format("float accumulator  : ${:.6f}  (error: {:.2e})\n",
                             static_cast<double>(ledger_float),
                             std::abs(static_cast<double>(ledger_float) - expected_fees));

    // -------------------------------------------------------------------------
    // Machine epsilon — smallest representable gap near 1.0
    // -------------------------------------------------------------------------
    std::cout << std::format("\ndouble epsilon (near 1.0): {:.2e}\n",
                             std::numeric_limits<double>::epsilon());
    std::cout << "Use double for pricing; reserve float for GPU/ SIMD bulk only "
                 "when error is bounded.\n";

    return 0;
}
