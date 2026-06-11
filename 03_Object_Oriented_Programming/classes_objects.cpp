/*
 * ============================================================================
 * MODULE 03 — LESSON 1: Classes, Objects, and Encapsulation
 * ============================================================================
 *
 * CONCEPT
 * -------
 * Object-oriented design bundles data (attributes) with behavior (methods) and
 * controls access via encapsulation. In quant libraries, this maps naturally:
 *
 *   class Instrument  — common identity (symbol, currency)
 *   class Bond        — coupon, face value, yield helpers
 *   class Equity      — shares, dividend yield
 *
 * Key OOP mechanics in C++:
 *   - Constructors initialize invariants (price > 0, maturity > 0)
 *   - `private` members prevent invalid state mutation from outside
 *   - `const` member functions promise not to modify observable state
 *   - Interface segregation: expose only what callers need
 *
 * FINANCIAL EXAMPLE
 * -----------------
 * We model a fixed-rate coupon bond and a dividend-paying equity, then build
 * a small `AssetLedger` that stores polymorphic *pointers-to-base* (preview of
 * Lesson 2) while computing total market value:
 *
 *   Bond dirty price ≈ Σ CFᵢ · exp(-y · tᵢ)   (simplified annual coupons)
 *   Equity value     = shares × spot
 *
 * BUILD
 * -----
 *   cmake --build build --target classes_objects
 *   ./build/03_Object_Oriented_Programming/classes_objects
 * ============================================================================
 */

#include <cmath>
#include <format>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace quant {

/// Abstract base — every tradable reports a mark-to-market value.
class Instrument {
public:
    explicit Instrument(std::string symbol, std::string currency = "USD")
        : symbol_(std::move(symbol))
        , currency_(std::move(currency)) {}

    virtual ~Instrument() = default;

    [[nodiscard]] std::string_view symbol() const noexcept { return symbol_; }
    [[nodiscard]] std::string_view currency() const noexcept { return currency_; }

    /// Present value in instrument currency.
    [[nodiscard]] virtual double mark_to_market() const = 0;

    [[nodiscard]] virtual std::string_view asset_class() const noexcept = 0;

protected:
    std::string symbol_;
    std::string currency_;
};

/// Fixed-coupon bond with annual payments (simplified textbook model).
class FixedRateBond final : public Instrument {
public:
    FixedRateBond(std::string symbol,
                  double       face_value,
                  double       coupon_rate,   // annual, as decimal (0.05 = 5%)
                  int          years_to_maturity,
                  double       yield_to_maturity)
        : Instrument(std::move(symbol))
        , face_value_(face_value)
        , coupon_rate_(coupon_rate)
        , years_to_maturity_(years_to_maturity)
        , ytm_(yield_to_maturity) {
        if (face_value_ <= 0.0) {
            throw std::invalid_argument("face_value must be positive");
        }
        if (years_to_maturity_ <= 0) {
            throw std::invalid_argument("years_to_maturity must be positive");
        }
    }

    [[nodiscard]] double face_value() const noexcept { return face_value_; }
    [[nodiscard]] double coupon_rate() const noexcept { return coupon_rate_; }
    [[nodiscard]] double yield_to_maturity() const noexcept { return ytm_; }

    [[nodiscard]] std::string_view asset_class() const noexcept override {
        return "FixedRateBond";
    }

    /// Dirty price: PV of coupons + redemption at yield y.
    [[nodiscard]] double mark_to_market() const override {
        const double coupon_payment = face_value_ * coupon_rate_;
        double       pv             = 0.0;

        for (int t = 1; t <= years_to_maturity_; ++t) {
            pv += coupon_payment * std::exp(-ytm_ * static_cast<double>(t));
        }
        pv += face_value_ * std::exp(-ytm_ * static_cast<double>(years_to_maturity_));
        return pv;
    }

private:
    double face_value_;
    double coupon_rate_;
    int    years_to_maturity_;
    double ytm_;
};

/// Cash equity position.
class Equity final : public Instrument {
public:
    Equity(std::string symbol, double shares, double spot, double dividend_yield = 0.0)
        : Instrument(std::move(symbol))
        , shares_(shares)
        , spot_(spot)
        , dividend_yield_(dividend_yield) {
        if (shares_ <= 0.0 || spot_ <= 0.0) {
            throw std::invalid_argument("shares and spot must be positive");
        }
    }

    [[nodiscard]] double shares() const noexcept { return shares_; }
    [[nodiscard]] double spot() const noexcept { return spot_; }
    [[nodiscard]] double dividend_yield() const noexcept { return dividend_yield_; }

    [[nodiscard]] std::string_view asset_class() const noexcept override {
        return "Equity";
    }

    [[nodiscard]] double mark_to_market() const override {
        return shares_ * spot_;
    }

private:
    double shares_;
    double spot_;
    double dividend_yield_;
};

/// Aggregates instruments — owns them via unique_ptr (Module 02).
class AssetLedger {
public:
    void add(std::unique_ptr<Instrument> instrument) {
        total_nmv_ += instrument->mark_to_market();
        holdings_.push_back(std::move(instrument));
    }

    [[nodiscard]] double total_net_market_value() const noexcept { return total_nmv_; }

    void print() const {
        std::cout << std::format("{:<8} {:<14} {:>12} {:>14}\n",
                                 "Symbol", "AssetClass", "Currency", "MTM");
        std::cout << std::string(52, '-') << '\n';

        for (const auto& inst : holdings_) {
            std::cout << std::format("{:<8} {:<14} {:>12} {:>14.2f}\n",
                                     inst->symbol(),
                                     inst->asset_class(),
                                     inst->currency(),
                                     inst->mark_to_market());
        }
    }

private:
    std::vector<std::unique_ptr<Instrument>> holdings_;
    double                                   total_nmv_{0.0};
};

}  // namespace quant

int main() {
    using namespace quant;

    std::cout << "=== Module 03: Classes & Objects ===\n\n";

    AssetLedger ledger;

    // Encapsulated bond: 5% annual coupon, 3-year, YTM 4.2%
    ledger.add(std::make_unique<FixedRateBond>("US-T", 1'000.0, 0.05, 3, 0.042));

    // Encapsulated equity: 500 shares @ 412.80
    ledger.add(std::make_unique<Equity>("MSFT", 500.0, 412.80, 0.007));

    // Second bond with different cash-flow profile
    ledger.add(std::make_unique<FixedRateBond>("CORP-X", 100'000.0, 0.0625, 5, 0.055));

    ledger.print();

    std::cout << std::format("\nTotal net market value : ${:.2f}\n",
                             ledger.total_net_market_value());

    std::cout << "\nEncapsulation ensures invalid bonds/equities throw at construction,\n"
                 "not silently corrupt desk risk.\n";

    return 0;
}
