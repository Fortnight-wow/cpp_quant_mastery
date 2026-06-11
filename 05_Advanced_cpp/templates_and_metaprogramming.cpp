/*
 * ============================================================================
 * MODULE 05 — LESSON 1: Templates, Concepts & Compile-Time Metaprogramming
 * ============================================================================
 *
 * CONCEPT
 * -------
 * Templates allow type- and value-parameterized code — essential for generic
 * numerics, policy-based design, and zero-overhead abstractions in quant:
 *
 *   - Function templates   — generic algorithms (e.g., clamp, lerp)
 *   - Class templates      — type-safe containers (e.g., Matrix<T>)
 *   - C++20 concepts       — constraints that produce readable errors
 *   - constexpr / if constexpr — compile-time branching for performance
 *
 * A concept constrains template parameters:
 *
 *   template <std::floating_point T>
 *   T black_scholes(...)
 *
 * This fails at the call site if T is int, with a clear diagnostic.
 *
 * FINANCIAL EXAMPLE
 * -----------------
 * 1) A constexpr "fast pow" for integer exponents (compile-time discount).
 * 2) A generic `delta` function constrained to floating-point types.
 * 3) A compile-time factorial (preview of template metaprogramming).
 * 4) A Numeric concept and policy-based `PricerTraits` for strategy selection.
 *
 * BUILD
 * -----
 *   cmake --build build --target templates_and_metaprogramming
 *   ./build/05_Advanced_CPP/templates_and_metaprogramming
 * ============================================================================
 */

#include <cmath>
#include <concepts>
#include <format>
#include <iostream>
#include <numbers>
#include <type_traits>
#include <vector>

namespace quant {

// -----------------------------------------------------------------------------
// 1. constexpr utility — compile-time discount factor
// -----------------------------------------------------------------------------

/// Compile-time integer exponent (fast pow for e.g. compounding periods).
[[nodiscard]] constexpr double constexpr_pow(double base, int exp) {
    if (exp < 0) {
        return 1.0 / constexpr_pow(base, -exp);
    }
    double result = 1.0;
    for (int i = 0; i < exp; ++i) {
        result *= base;
    }
    return result;
}

static_assert(constexpr_pow(1.05, 3) > 1.1576 && constexpr_pow(1.05, 3) < 1.1577,
              "constexpr_pow(1.05, 3) should be ~1.157625");

/// Compile-time factorial — template metaprogramming classic.
template <int N>
struct Factorial {
    static constexpr int value = N * Factorial<N - 1>::value;
};

template <>
struct Factorial<0> {
    static constexpr int value = 1;
};

static_assert(Factorial<5>::value == 120);
static_assert(Factorial<7>::value == 5040);

// -----------------------------------------------------------------------------
// 2. Concepts for quant code
// -----------------------------------------------------------------------------

/// A number type suitable for pricing — must be floating-point.
template <typename T>
concept PricingNumeric = std::floating_point<T>;

/// A number type suitable for counting — must be integral and non-negative.
template <typename T>
concept Countable = std::integral<T> && std::unsigned_integral<T>;

// -----------------------------------------------------------------------------
// 3. Generic delta function constrained to floating-point types
// -----------------------------------------------------------------------------

/// Normal CDF — duplicated here so the file is self-contained.
[[nodiscard]] inline double norm_cdf(double x) {
    return 0.5 * (1.0 + std::erf(x / std::numbers::sqrt2));
}

/// Black-Scholes delta, generic over any floating-point pricing type.
template <PricingNumeric T>
[[nodiscard]] T delta(T spot, T strike, T rate, T vol, T time, bool is_call) {
    if (time <= T{0}) {
        if (is_call) {
            return spot > strike ? T{1} : T{0};
        }
        return spot < strike ? T{-1} : T{0};
    }

    const T d1 = (std::log(spot / strike) + (rate + T{0.5} * vol * vol) * time) /
                 (vol * std::sqrt(time));

    const T nd1 = static_cast<T>(norm_cdf(static_cast<double>(d1)));
    if (is_call) {
        return nd1;
    }
    return nd1 - T{1};
}

// -----------------------------------------------------------------------------
// 4. Policy-based traits — select pricing formula at compile time
// -----------------------------------------------------------------------------

enum class PricingModel { BlackScholes, Bachelier };

template <PricingModel M>
struct PricerTraits;

template <>
struct PricerTraits<PricingModel::BlackScholes> {
    static constexpr const char* name = "Black-Scholes";
    /// Returns the standard Black-Scholes delta.
    template <PricingNumeric T>
    [[nodiscard]] static T delta(T s, T k, T r, T v, T t, bool call) {
        return quant::delta(s, k, r, v, t, call);
    }
};

template <>
struct PricerTraits<PricingModel::Bachelier> {
    static constexpr const char* name = "Bachelier (normal)";
    /// Bachelier delta for normal model — simplified for demonstration.
    template <PricingNumeric T>
    [[nodiscard]] static T delta(T /*spot*/, T /*strike*/, T /*rate*/,
                                  T /*vol*/, T /*time*/, bool call) {
        // In the normal (Bachelier) model delta ≈ N(d1) for calls.
        // For this demo we return the same shape to show policy dispatch.
        return call ? T{0.6} : T{-0.4};
    }
};

// -----------------------------------------------------------------------------
// 5. if constexpr — compile-time dispatch on instrument type tags
// -----------------------------------------------------------------------------

enum class InstrumentTag { Equity, Bond, Option };

template <InstrumentTag Tag>
[[nodiscard]] std::string_view asset_class_name() {
    if constexpr (Tag == InstrumentTag::Equity) {
        return "Equity";
    } else if constexpr (Tag == InstrumentTag::Bond) {
        return "Fixed Income";
    } else if constexpr (Tag == InstrumentTag::Option) {
        return "Derivative";
    }
}

}  // namespace quant

int main() {
    using namespace quant;

    std::cout << "=== Module 05: Templates & Metaprogramming ===\n\n";

    // -------------------------------------------------------------------------
    // 1. Constexpr pow — compile-time discounting
    // -------------------------------------------------------------------------
    constexpr double rate      = 0.05;
    constexpr int    years     = 5;
    constexpr double df        = 1.0 / constexpr_pow(1.0 + rate, years);
    constexpr double pv_of_100 = 100.0 * df;

    std::cout << std::format("constexpr discount factor ({} yrs @ {:.1f}%): {:.6f}\n",
                             years, rate * 100.0, df);
    std::cout << std::format("PV of $100: {:.4f}\n", pv_of_100);

    // -------------------------------------------------------------------------
    // 2. Compile-time factorial (template metaprogramming)
    // -------------------------------------------------------------------------
    std::cout << std::format("Factorial<7>::value = {}\n", Factorial<7>::value);
    std::cout << std::format("Factorial<10>::value = {}\n", Factorial<10>::value);

    // -------------------------------------------------------------------------
    // 3. Generic delta with concept enforcement
    // -------------------------------------------------------------------------
    const double call_delta = delta(175.50, 180.00, 0.045, 0.22, 0.25, true);
    const double put_delta  = delta(175.50, 180.00, 0.045, 0.22, 0.25, false);

    std::cout << std::format("\nBlack-Scholes delta (S=175.50, K=180.00, T=0.25y)\n");
    std::cout << std::format("  Call delta (double) : {:.6f}\n", call_delta);
    std::cout << std::format("  Put delta  (double) : {:.6f}\n", put_delta);

    // float version — same template, different instantiation
    const float call_delta_f = delta(175.50F, 180.00F, 0.045F, 0.22F, 0.25F, true);
    std::cout << std::format("  Call delta (float)  : {:.6f}\n",
                             static_cast<double>(call_delta_f));

    // -------------------------------------------------------------------------
    // 4. Policy-based traits dispatch
    // -------------------------------------------------------------------------
    std::cout << "\nPolicy-based pricing traits:\n";

    using BS = PricerTraits<PricingModel::BlackScholes>;
    std::cout << std::format("  {} delta: {:.6f}\n",
                             BS::name, BS::delta(175.50, 180.00, 0.045, 0.22, 0.25, true));

    using Bach = PricerTraits<PricingModel::Bachelier>;
    std::cout << std::format("  {} delta: {:.6f}\n",
                             Bach::name, Bach::delta(175.50, 180.00, 0.045, 0.22, 0.25, true));

    // -------------------------------------------------------------------------
    // 5. if constexpr tag dispatch
    // -------------------------------------------------------------------------
    std::cout << "\nif constexpr tag dispatch:\n";
    std::cout << std::format("  InstrumentTag::Equity  → {}\n", asset_class_name<InstrumentTag::Equity>());
    std::cout << std::format("  InstrumentTag::Bond    → {}\n", asset_class_name<InstrumentTag::Bond>());
    std::cout << std::format("  InstrumentTag::Option  → {}\n", asset_class_name<InstrumentTag::Option>());

    std::cout << "\nTemplates and concepts catch type errors at compile time,\n"
                 "not runtime — critical for production pricing libraries.\n";

    return 0;
}
