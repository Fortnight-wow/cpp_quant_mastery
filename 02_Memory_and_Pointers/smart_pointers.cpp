/*
 * ============================================================================
 * MODULE 02 — LESSON 2: Smart Pointers — Ownership in Pricing Libraries
 * ============================================================================
 *
 * CONCEPT
 * -------
 * Smart pointers wrap raw pointers with deterministic lifetime management
 * (RAII — Resource Acquisition Is Initialization):
 *
 *   std::unique_ptr<T>  — Exclusive ownership. Move-only. Zero overhead vs raw
 *                         pointer when used correctly. Default for heap objects.
 *
 *   std::shared_ptr<T>  — Shared ownership via reference counting. Use when
 *                         multiple objects must reference the same resource
 *                         (e.g., one Underlying shared by many Option instances).
 *
 * Factory functions `std::make_unique` / `std::make_shared` are preferred:
 *   - Exception-safe construction
 *   - Single allocation for make_shared (control block + object)
 *
 * FINANCIAL EXAMPLE
 * -----------------
 * A mini option book:
 *   - `Underlying` (spot, dividend yield) is shared across three vanillas
 *   - Each `VanillaOption` is exclusively owned by the portfolio via unique_ptr
 *   - We price each contract with intrinsic value max(S-K,0) for calls (demo)
 *
 * Portfolio mark-to-market:
 *
 *   MTM = Σ intrinsic(S, Kᵢ, typeᵢ)
 *
 * This mirrors real desk structure: shared market data, uniquely owned positions.
 *
 * BUILD
 * -----
 *   cmake --build build --target smart_pointers
 *   ./build/02_Memory_and_Pointers/smart_pointers
 * ============================================================================
 */

#include <cmath>
#include <format>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace quant {

enum class OptionType { Call, Put };

/// Shared market data — many instruments reference the same underlying.
struct Underlying {
    std::string    symbol;
    double         spot;
    double         dividend_yield;  // continuous q

    [[nodiscard]] double forward_price(double rate, double time) const {
        // F = S · exp((r - q) T)
        return spot * std::exp((rate - dividend_yield) * time);
    }
};

/// Single vanilla option — owned exclusively by the portfolio container.
class VanillaOption {
public:
    VanillaOption(std::shared_ptr<const Underlying> underlying,
                  OptionType                        type,
                  double                            strike,
                  double                            maturity_years)
        : underlying_(std::move(underlying))
        , type_(type)
        , strike_(strike)
        , maturity_years_(maturity_years) {}

    [[nodiscard]] OptionType type() const noexcept { return type_; }
    [[nodiscard]] double     strike() const noexcept { return strike_; }
    [[nodiscard]] double     maturity_years() const noexcept { return maturity_years_; }

    [[nodiscard]] std::string_view underlying_symbol() const {
        return underlying_ ? underlying_->symbol : std::string_view{};
    }

    /// Intrinsic value (not time value) — pedagogical placeholder before Module 07.
    [[nodiscard]] double intrinsic() const {
        if (!underlying_) {
            return 0.0;
        }

        const double S = underlying_->spot;
        const double K = strike_;

        if (type_ == OptionType::Call) {
            return std::max(S - K, 0.0);
        }
        return std::max(K - S, 0.0);
    }

    [[nodiscard]] const Underlying& underlying() const { return *underlying_; }

private:
    std::shared_ptr<const Underlying> underlying_;
    OptionType                        type_;
    double                            strike_;
    double                            maturity_years_;
};

class Portfolio {
public:
    void add_option(std::unique_ptr<VanillaOption> option) {
        total_mtm_ += option->intrinsic();
        options_.push_back(std::move(option));
    }

    [[nodiscard]] double total_mtm() const noexcept { return total_mtm_; }
    [[nodiscard]] std::size_t size() const noexcept { return options_.size(); }

    void print_book() const {
        std::cout << std::format("{:<6} {:<6} {:>8} {:>8} {:>12}\n",
                                 "Symbol", "Type", "Strike", "T(yrs)", "Intrinsic");
        std::cout << std::string(46, '-') << '\n';

        for (const auto& opt_ptr : options_) {
            const VanillaOption& opt = *opt_ptr;
            const char* type_str = (opt.type() == OptionType::Call) ? "CALL" : "PUT ";

            std::cout << std::format("{:<6} {:<6} {:>8.2f} {:>8.2f} {:>12.4f}\n",
                                     opt.underlying_symbol(),
                                     type_str,
                                     opt.strike(),
                                     opt.maturity_years(),
                                     opt.intrinsic());
        }
    }

private:
    std::vector<std::unique_ptr<VanillaOption>> options_;
    double total_mtm_{0.0};
};

}  // namespace quant

int main() {
    using namespace quant;

    std::cout << "=== Module 02: Smart Pointers ===\n\n";

    // -------------------------------------------------------------------------
    // 1. Shared ownership of market data (one Underlying, many options)
    // -------------------------------------------------------------------------
    std::shared_ptr<Underlying> aapl = std::make_shared<Underlying>(
        Underlying{.symbol = "AAPL", .spot = 175.50, .dividend_yield = 0.008});

    std::cout << "Underlying ref count (expect 1): "
              << aapl.use_count() << '\n';

    // -------------------------------------------------------------------------
    // 2. unique_ptr for each position — exclusive ownership transfers to portfolio
    // -------------------------------------------------------------------------
    Portfolio book;

    {
        auto call_180 = std::make_unique<VanillaOption>(
            aapl, OptionType::Call, 180.0, 0.25);
        auto put_170  = std::make_unique<VanillaOption>(
            aapl, OptionType::Put, 170.0, 0.50);
        auto call_165 = std::make_unique<VanillaOption>(
            aapl, OptionType::Call, 165.0, 0.10);

        std::cout << "Underlying ref count after wiring 3 options (expect 4): "
                  << aapl.use_count() << "\n\n";

        book.add_option(std::move(call_180));
        book.add_option(std::move(put_170));
        book.add_option(std::move(call_165));

        // moved-from unique_ptrs are empty — no double ownership
        if (!call_180) {
            std::cout << "call_180 successfully moved into portfolio (nullptr)\n\n";
        }
    }

    // -------------------------------------------------------------------------
    // 3. Mark-to-market report
    // -------------------------------------------------------------------------
    std::cout << "=== Option Book (intrinsic MTM demo) ===\n";
    book.print_book();

    std::cout << std::format("\nPositions held : {}\n", book.size());
    std::cout << std::format("Total intrinsic: {:.4f}\n", book.total_mtm());

    // -------------------------------------------------------------------------
    // 4. Forward price using shared underlying (still alive while options exist)
    // -------------------------------------------------------------------------
    constexpr double risk_free_rate = 0.045;
    const double     forward        = aapl->forward_price(risk_free_rate, 0.25);

    std::cout << std::format("\nShared Underlying: {} spot {:.2f}\n",
                             aapl->symbol,
                             aapl->spot);
    std::cout << std::format("3-month forward F = S·exp((r-q)T) = {:.4f}\n", forward);
    std::cout << std::format("Underlying ref count (book + local): {}\n",
                             aapl.use_count());

    // -------------------------------------------------------------------------
    // 5. Ownership summary
    // -------------------------------------------------------------------------
    std::cout << "\n--- Ownership cheat sheet ---\n";
    std::cout << "  unique_ptr → one owner; move to transfer; stack-like clarity.\n";
    std::cout << "  shared_ptr → many owners; use for shared market data caches.\n";
    std::cout << "  Raw T*     → non-owning view; valid only while owner lives.\n";

    return 0;
}
