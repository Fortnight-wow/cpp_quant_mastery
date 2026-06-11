/*
 * ============================================================================
 * MODULE 01 — LESSON 1: Hello World & The Quant Build Pipeline
 * ============================================================================
 *
 * CONCEPT
 * -------
 * Every pricing library, risk engine, and matching engine begins as a single
 * translation unit compiled into an executable or shared object. Understanding
 * the minimal C++ program structure is the first step toward production quant
 * code:
 *
 *   Source (.cpp)  -->  Compiler  -->  Object file  -->  Linker  -->  Binary
 *
 * In quantitative finance we routinely rebuild after changing:
 *   - Model parameters (vol surfaces, rate curves)
 *   - Numerical schemes (tree steps, MC paths)
 *   - Latency-critical order-book logic
 *
 * This lesson introduces:
 *   - The `#include` preprocessor directive (pulling in standard library headers)
 *   - The `main()` entry point (where execution begins)
 *   - `std::cout` for structured console output (logging precursor)
 *   - C++20 `std::format` for type-safe, readable formatting
 *
 * FINANCIAL CONTEXT
 * -----------------
 * Before pricing a European call with Black–Scholes we need a toolchain that
 * compiles cleanly with `-std=c++20`. This program confirms your environment
 * and prints a sample option quote placeholder—the same I/O pattern used in
 * desk sanity checks and batch risk runs.
 *
 * BUILD
 * -----
 *   cmake --build build --target hello_world
 *   ./build/01_Basics/hello_world
 * ============================================================================
 */

#include <format>
#include <iostream>
#include <string>
#include <string_view>

int main() {
    // -------------------------------------------------------------------------
    // 1. Classic stream output — still ubiquitous in legacy trading systems
    // -------------------------------------------------------------------------
    std::cout << "Hello, Quant World!\n";
    std::cout << "cpp-quant-mastery | Module 01: Basics\n\n";

    // -------------------------------------------------------------------------
    // 2. C++20 std::format — preferred for readable, locale-aware reporting
    // -------------------------------------------------------------------------
    constexpr std::string_view underlying{"AAPL"};
    constexpr double       spot{175.50};
    constexpr double       strike{180.00};
    constexpr double       time_to_expiry_years{0.25};  // ~3 months
    constexpr double       risk_free_rate{0.045};       // 4.5% continuous
    constexpr double       volatility{0.22};            // 22% annualized

    const std::string header = std::format(
        "Sample option ticket (placeholder — Module 07 prices this exactly):\n"
        "  Underlying : {}\n"
        "  Spot (S)   : {:.2f}\n"
        "  Strike (K) : {:.2f}\n"
        "  T (years)  : {:.4f}\n"
        "  r          : {:.2f}% (continuous, {:.4f})\n"
        "  sigma      : {:.0f}% ({:.2f} annualized)\n",
        underlying,
        spot,
        strike,
        time_to_expiry_years,
        risk_free_rate * 100.0,
        risk_free_rate,
        volatility * 100.0,
        volatility);

    std::cout << header << '\n';

    // -------------------------------------------------------------------------
    // 3. Return code convention: 0 = success (Unix shell / CI pipelines)
    // -------------------------------------------------------------------------
    std::cout << "Build OK. Ready for data types, control flow, and pricing.\n";
    return 0;
}
