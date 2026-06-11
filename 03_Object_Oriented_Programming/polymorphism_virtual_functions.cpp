/*
 * ============================================================================
 * MODULE 03 — LESSON 2: Polymorphism & Virtual Functions
 * ============================================================================
 *
 * CONCEPT
 * -------
 * Polymorphism lets code operate on abstract interfaces while concrete models
 * supply different implementations—the Strategy pattern behind quant pricing:
 *
 *   Pricer (abstract)  ──► BlackScholesPricer
 *                      ──► BinomialPricer      (Module 07)
 *                      ──► MonteCarloPricer
 *
 * C++ mechanism:
 *   - Pure virtual `= 0` defines an interface
 *   - `override` catches signature mistakes at compile time
 *   - Virtual destructor ensures derived cleanup through base pointer
 *   - Dynamic dispatch: `pricer->price(option)` resolves at runtime
 *
 * Without `virtual`, calling through `Pricer*` would slice or bind to base.
 *
 * FINANCIAL EXAMPLE
 * -----------------
 * Two concrete pricers for a European call:
 *
 *   1) BlackScholesPricer — closed-form N(d₁), N(d₂) formula
 *   2) IntrinsicPricer    — lower bound max(S-K, 0) for sanity checks
 *
 * Black–Scholes European call:
 *
 *   d₁ = [ln(S/K) + (r + ½σ²)T] / (σ√T)
 *   d₂ = d₁ - σ√T
 *   C  = S·N(d₁) - K·e^{-rT}·N(d₂)
 *
 * We iterate a `vector<Pricer*>` (non-owning views) to mimic a risk batch.
 *
 * BUILD
 * -----
 *   cmake --build build --target polymorphism_virtual_functions
 *   ./build/03_Object_Oriented_Programming/polymorphism_virtual_functions
 * ============================================================================
 */

#include <cmath>
#include <format>
#include <iostream>
#include <memory>
#include <numbers>
#include <string>
#include <string_view>
#include <vector>

namespace quant {

enum class OptionStyle { European, American };
enum class OptionType  { Call, Put };

/// Minimal option contract passed into pricers.
struct EuropeanVanilla {
    OptionType type{OptionType::Call};
    double     spot{0.0};
    double     strike{0.0};
    double     rate{0.0};       // continuous r
    double     volatility{0.0}; // annual sigma
    double     time_years{0.0};
};

/// Standard normal CDF via erf — same pattern as Module 07.
[[nodiscard]] inline double norm_cdf(double x) {
    return 0.5 * (1.0 + std::erf(x / std::numbers::sqrt2));
}

/// Abstract pricing interface — extend with new models without changing clients.
class Pricer {
public:
    virtual ~Pricer() = default;

    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    [[nodiscard]] virtual double price(const EuropeanVanilla& option) const = 0;
};

/// Closed-form Black–Scholes for European vanillas.
class BlackScholesPricer final : public Pricer {
public:
    [[nodiscard]] std::string_view name() const noexcept override {
        return "BlackScholes";
    }

    [[nodiscard]] double price(const EuropeanVanilla& opt) const override {
        const double S = opt.spot;
        const double K = opt.strike;
        const double r = opt.rate;
        const double v = opt.volatility;
        const double T = opt.time_years;

        if (T <= 0.0) {
            return intrinsic(opt);
        }

        const double sqrt_T = std::sqrt(T);
        const double d1     = (std::log(S / K) + (r + 0.5 * v * v) * T) / (v * sqrt_T);
        const double d2     = d1 - v * sqrt_T;

        if (opt.type == OptionType::Call) {
            return S * norm_cdf(d1) - K * std::exp(-r * T) * norm_cdf(d2);
        }

        // Put via put-call parity is also fine; direct formula shown here.
        return K * std::exp(-r * T) * norm_cdf(-d2) - S * norm_cdf(-d1);
    }

private:
    [[nodiscard]] static double intrinsic(const EuropeanVanilla& opt) {
        if (opt.type == OptionType::Call) {
            return std::max(opt.spot - opt.strike, 0.0);
        }
        return std::max(opt.strike - opt.spot, 0.0);
    }
};

/// Trivial lower-bound pricer — useful for model validation pipelines.
class IntrinsicPricer final : public Pricer {
public:
    [[nodiscard]] std::string_view name() const noexcept override {
        return "Intrinsic";
    }

    [[nodiscard]] double price(const EuropeanVanilla& opt) const override {
        if (opt.type == OptionType::Call) {
            return std::max(opt.spot - opt.strike, 0.0);
        }
        return std::max(opt.strike - opt.spot, 0.0);
    }
};

/// Batch pricing engine — accepts any Pricer implementation.
class PricingBatch {
public:
    void register_pricer(Pricer* pricer) {
        if (pricer != nullptr) {
            pricers_.push_back(pricer);
        }
    }

    void run(const EuropeanVanilla& option) const {
        std::cout << std::format(
            "Contract: {} K={:.2f} S={:.2f} T={:.2f}y r={:.2f}% sigma={:.0f}%\n",
            (option.type == OptionType::Call ? "CALL" : "PUT"),
            option.strike,
            option.spot,
            option.time_years,
            option.rate * 100.0,
            option.volatility * 100.0);

        for (const Pricer* pricer : pricers_) {
            const double px = pricer->price(option);
            std::cout << std::format("  {:<14} price = {:.6f}\n", pricer->name(), px);
        }
        std::cout << '\n';
    }

private:
    std::vector<Pricer*> pricers_;
};

}  // namespace quant

int main() {
    using namespace quant;

    std::cout << "=== Module 03: Polymorphism & Virtual Functions ===\n\n";

    // Concrete pricers live on the stack; batch holds non-owning Pricer*
    BlackScholesPricer bs;
    IntrinsicPricer    intrinsic;

    PricingBatch batch;
    batch.register_pricer(&bs);
    batch.register_pricer(&intrinsic);

    // AAPL-like European call — same ticket as Module 01 hello_world
    const EuropeanVanilla call_ticket{
        .type       = OptionType::Call,
        .spot       = 175.50,
        .strike     = 180.00,
        .rate       = 0.045,
        .volatility = 0.22,
        .time_years = 0.25,
    };

    batch.run(call_ticket);

    // OTM put on same underlying — demonstrates virtual put branch
    const EuropeanVanilla put_ticket{
        .type       = OptionType::Put,
        .spot       = 175.50,
        .strike     = 170.00,
        .rate       = 0.045,
        .volatility = 0.22,
        .time_years = 0.50,
    };

    batch.run(put_ticket);

    // -------------------------------------------------------------------------
    // Virtual destructor demo — safe deletion through base pointer
    // -------------------------------------------------------------------------
    std::unique_ptr<Pricer> owned = std::make_unique<BlackScholesPricer>();
    std::cout << std::format(
        "Polymorphic unique_ptr<Pricer> via make_unique<BlackScholesPricer>\n"
        "  {} price (call) = {:.6f}\n",
        owned->name(),
        owned->price(call_ticket));

    std::cout << "\nVirtual dispatch lets risk systems swap models without "
                 "recompiling downstream code.\n";

    return 0;
}
