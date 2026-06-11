/*
 * ============================================================================
 * MODULE 02 — LESSON 1: Raw Pointers, Stack vs Heap, and Manual Lifetime
 * ============================================================================
 *
 * CONCEPT
 * -------
 * Memory in C++ falls into two primary regions relevant to quant systems:
 *
 *   STACK  — Automatic storage tied to scope (local variables, fixed arrays).
 *            Fast allocation/deallocation; size often known at compile time.
 *
 *   HEAP   — Dynamic storage via `new` / `delete` (or `new[]` / `delete[]`).
 *            Size determined at runtime (e.g., number of ticks in a session).
 *
 * A raw pointer (`T*`) stores an address. It does NOT own the memory it
 * points to unless your team convention says otherwise. Manual `delete` is
 * error-prone:
 *
 *   - Memory leaks (forget to delete)
 *   - Double-free (delete twice)
 *   - Dangling pointers (use after free)
 *
 * Production quant code rarely relies on naked `new`/`delete`. This lesson
 * shows the legacy pattern so you can read older libraries—and understand
 * why smart pointers (next lesson) and RAII dominate modern codebases.
 *
 * FINANCIAL EXAMPLE
 * -----------------
 * At market open we don't know how many mid-price ticks will arrive. We
 * allocate a heap buffer sized to the session, fill it with synthetic
 * tick data, compute a simple VWAP, then explicitly release memory.
 *
 * VWAP (Volume-Weighted Average Price):
 *
 *   VWAP = Σ (pᵢ · vᵢ) / Σ vᵢ
 *
 * BUILD
 * -----
 *   cmake --build build --target raw_pointers
 *   ./build/02_Memory_and_Pointers/raw_pointers
 * ============================================================================
 */

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <format>
#include <iostream>
#include <limits>
#include <span>
#include <utility>
#include <vector>

namespace {

struct Tick {
    double price;
    double volume;
};

/// Compute VWAP over [begin, end). Returns NaN if total volume is zero.
[[nodiscard]] double vwap(std::span<const Tick> ticks) {
    double pv_sum = 0.0;
    double v_sum  = 0.0;

    for (const Tick& t : ticks) {
        pv_sum += t.price * t.volume;
        v_sum  += t.volume;
    }

    if (v_sum <= 0.0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return pv_sum / v_sum;
}

/// Demonstrate stack allocation — lifetime ends when function returns.
[[nodiscard]] double stack_session_vwap(int num_ticks) {
    // Variable-length stack arrays are non-standard; use std::vector on stack
    // (the vector object is on the stack; its elements live on the heap).
    std::vector<Tick> ticks(static_cast<std::size_t>(num_ticks));

    for (int i = 0; i < num_ticks; ++i) {
        ticks[static_cast<std::size_t>(i)] = {
            100.0 + 0.05 * static_cast<double>(i),
            500.0 + static_cast<double>(i % 7) * 10.0,
        };
    }

    return vwap(ticks);
}

/// Legacy heap pattern: caller MUST call `delete[]` on success.
[[nodiscard]] double heap_session_vwap(int num_ticks, Tick** out_buffer, std::size_t* out_size) {
    if (num_ticks <= 0 || out_buffer == nullptr || out_size == nullptr) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    *out_buffer = nullptr;
    *out_size   = 0;

    // Raw heap allocation — size only known at runtime
    Tick* buffer = new Tick[static_cast<std::size_t>(num_ticks)];

    for (int i = 0; i < num_ticks; ++i) {
        buffer[static_cast<std::size_t>(i)] = {
            250.75 - 0.02 * static_cast<double>(i),
            1'000.0 + static_cast<double>((i * 3) % 11) * 25.0,
        };
    }

    *out_buffer = buffer;
    *out_size   = static_cast<std::size_t>(num_ticks);

    return vwap(std::span<const Tick>(buffer, static_cast<std::size_t>(num_ticks)));
}

}  // namespace

int main() {
    std::cout << "=== Module 02: Raw Pointers ===\n\n";

    // -------------------------------------------------------------------------
    // 1. Stack / RAII-friendly vector (preferred in modern code)
    // -------------------------------------------------------------------------
    constexpr int stack_ticks = 8;
    const double stack_vwap = stack_session_vwap(stack_ticks);

    std::cout << std::format("Stack-backed session ({} ticks)\n", stack_ticks);
    std::cout << std::format("  VWAP = {:.4f}\n\n", stack_vwap);

    // -------------------------------------------------------------------------
    // 2. Raw heap allocation — manual ownership
    // -------------------------------------------------------------------------
    Tick*       raw_buffer = nullptr;
    std::size_t raw_size   = 0;

    constexpr int heap_ticks = 12;
    const double heap_vwap = heap_session_vwap(heap_ticks, &raw_buffer, &raw_size);

    std::cout << std::format("Heap-backed session ({} ticks) via new[]\n", heap_ticks);
    std::cout << std::format("  Buffer address : {}\n", static_cast<void*>(raw_buffer));
    std::cout << std::format("  VWAP           : {:.4f}\n", heap_vwap);

    // Inspect first and last tick through the raw pointer (array decay)
    if (raw_buffer != nullptr && raw_size >= 2) {
        std::cout << std::format("  First tick     : price {:.2f}, vol {:.0f}\n",
                                 raw_buffer[0].price,
                                 raw_buffer[0].volume);
        std::cout << std::format("  Last tick      : price {:.2f}, vol {:.0f}\n",
                                 raw_buffer[raw_size - 1].price,
                                 raw_buffer[raw_size - 1].volume);
    }

    // CRITICAL: match new[] with delete[] — never delete (without brackets)
    delete[] raw_buffer;
    raw_buffer = nullptr;  // good practice: avoid accidental reuse

    std::cout << "\nMemory released with delete[]\n";

    // -------------------------------------------------------------------------
    // 3. Pointer arithmetic preview (fixed stack array — no manual free)
    // -------------------------------------------------------------------------
    double closes[] = {101.2, 101.5, 100.9, 102.1, 101.8};
    double* begin     = closes;
    double* end       = closes + std::size(closes);

    double sum = 0.0;
    int    n   = 0;
    for (double* p = begin; p != end; ++p) {
        sum += *p;  // dereference
        ++n;
    }
    const double simple_avg = sum / static_cast<double>(n);

    std::cout << std::format("\nPointer walk over stack closes[] (n={})\n", n);
    std::cout << std::format("  Simple average close = {:.4f}\n", simple_avg);

    // -------------------------------------------------------------------------
    // 4. Why raw ownership is fragile (conceptual — do NOT run broken code)
    // -------------------------------------------------------------------------
    std::cout << "\n--- Production guidance ---\n";
    std::cout << "  • Prefer std::vector / smart pointers over naked new/delete.\n";
    std::cout << "  • Raw pointers are OK as non-owning observers (T* view into data).\n";
    std::cout << "  • Next lesson: unique_ptr and shared_ptr encode ownership.\n";

    return 0;
}
