/*
 * ============================================================================
 * MODULE 05 — LESSON 2: Multithreading & Concurrency for Quant Simulation
 * ============================================================================
 *
 * CONCEPT
 * -------
 * Low-latency trading and Monte Carlo simulation demand concurrent execution:
 *
 *   std::thread     — OS-level thread; explicit join/detach management
 *   std::mutex      — mutual exclusion for shared book data
 *   std::atomic<T>  — lock-free reads/writes for single-counter updates
 *   std::jthread    — C++20 joining thread (auto-join on destruction)
 *
 * Pitfalls in quant code:
 *   - Data races on shared P&L accumulators (use atomic or local + reduce)
 *   - False sharing (pad counters to cache-line boundaries)
 *   - Deadlocks when multiple mutexes guard correlated books
 *
 * FINANCIAL EXAMPLE
 * -----------------
 * We compute a toy Monte Carlo estimate of π (stand-in for option price)
 * using parallel paths with:
 *   1. std::jthread + mutex for shared accumulator (naive — contention)
 *   2. std::jthread + local sums + final reduction (better — minimal sync)
 *   3. std::atomic for a global path counter (lock-free)
 *
 * π ≈ (4/N) · Σ 𝟙(x² + y² ≤ 1)   for uniform (x,y) in [0,1]²
 *
 * BUILD
 * -----
 *   cmake --build build --target multithreading_concurrency
 *   ./build/05_Advanced_CPP/multithreading_concurrency
 * ============================================================================
 */

#include <atomic>
#include <chrono>
#include <cmath>
#include <format>
#include <iostream>
#include <limits>
#include <numbers>
#include <mutex>
#include <random>
#include <span>
#include <thread>
#include <vector>

namespace quant {

/// Estimate π via Monte Carlo in [0,1]² — one thread's work.
[[nodiscard]] double estimate_pi_chunk(std::uint64_t points,
                                        std::uint64_t seed_offset) {
    std::mt19937_64 rng{
        static_cast<std::uint64_t>(std::random_device{}()) + seed_offset};
    std::uniform_real_distribution<double> dist{0.0, 1.0};

    std::uint64_t inside = 0;
    for (std::uint64_t i = 0; i < points; ++i) {
        const double x = dist(rng);
        const double y = dist(rng);
        if (x * x + y * y <= 1.0) {
            ++inside;
        }
    }
    return 4.0 * static_cast<double>(inside) / static_cast<double>(points);
}

/// Shared counter approach — mutex protects the total (pedagogical only).
[[nodiscard]] double pi_mutex_approach(std::uint64_t total_points,
                                        unsigned int num_threads) {
    std::mutex           mtx;
    double               total_estimate = 0.0;
    std::vector<std::jthread> workers;
    workers.reserve(num_threads);

    const auto chunk = total_points / static_cast<std::uint64_t>(num_threads);
    for (unsigned int t = 0; t < num_threads; ++t) {
        workers.emplace_back([&, t]() {
            const double local = estimate_pi_chunk(chunk, t * 12345);
            std::lock_guard<std::mutex> lock(mtx);
            total_estimate += local;
        });
    }
    workers.clear();  // join all before reading result

    const double avg = total_estimate / static_cast<double>(num_threads);
    return avg;
}

/// Per-thread local sum + final reduction — minimal synchronization.
[[nodiscard]] double pi_reduce_approach(std::uint64_t total_points,
                                         unsigned int num_threads) {
    std::vector<double>           results(num_threads, 0.0);
    std::vector<std::jthread>     workers;
    workers.reserve(num_threads);

    const auto chunk = total_points / static_cast<std::uint64_t>(num_threads);
    for (unsigned int t = 0; t < num_threads; ++t) {
        workers.emplace_back([&, t]() {
            results[t] = estimate_pi_chunk(chunk, t * 67890);
        });
    }
    workers.clear();

    double sum = 0.0;
    for (const double r : results) {
        sum += r;
    }
    return sum / static_cast<double>(num_threads);
}

/// Atomic counter — lock-free progress tracking.
[[nodiscard]] double pi_atomic_counter(std::uint64_t total_points,
                                        unsigned int num_threads) {
    std::atomic<std::uint64_t> inside_count{0};
    std::vector<std::jthread>  workers;
    workers.reserve(num_threads);

    const auto chunk = total_points / static_cast<std::uint64_t>(num_threads);
    for (unsigned int t = 0; t < num_threads; ++t) {
        workers.emplace_back([&, t]() {
            std::mt19937_64 rng{
                static_cast<std::uint64_t>(std::random_device{}()) + t * 11111};
            std::uniform_real_distribution<double> dist{0.0, 1.0};

            std::uint64_t local_inside = 0;
            for (std::uint64_t i = 0; i < chunk; ++i) {
                const double x = dist(rng);
                const double y = dist(rng);
                if (x * x + y * y <= 1.0) {
                    ++local_inside;
                }
            }
            inside_count.fetch_add(local_inside, std::memory_order_relaxed);
        });
    }
    workers.clear();

    const double total_pts = static_cast<double>(num_threads * chunk);
    return 4.0 * static_cast<double>(inside_count.load(std::memory_order_relaxed)) / total_pts;
}

}  // namespace quant

int main() {
    using namespace quant;

    std::cout << "=== Module 05: Multithreading & Concurrency ===\n\n";

    constexpr std::uint64_t total_points  = 16'000'000;
    constexpr unsigned int  num_threads   = 4;

    // Warmup / info
    std::cout << std::format("Estimating π via Monte Carlo\n");
    std::cout << std::format("  Total points : {}\n", total_points);
    std::cout << std::format("  Threads      : {}\n", num_threads);
    std::cout << std::format("  Points/core  : {}\n\n",
                             total_points / static_cast<std::uint64_t>(num_threads));

    // -------------------------------------------------------------------------
    // 1. Mutex approach (naive shared accumulator — contention)
    // -------------------------------------------------------------------------
    {
        const auto t0 = std::chrono::steady_clock::now();
        const double pi = pi_mutex_approach(total_points, num_threads);
        const auto t1 = std::chrono::steady_clock::now();
        const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        std::cout << std::format("1. Mutex (shared accumulator)\n");
        std::cout << std::format("   π ≈ {:.6f}  (error: {:.2e})  [{:.1f} ms]\n",
                                 pi, std::abs(pi - std::numbers::pi), ms);
    }

    // -------------------------------------------------------------------------
    // 2. Local sum + reduce (minimal synchronization)
    // -------------------------------------------------------------------------
    {
        const auto t0 = std::chrono::steady_clock::now();
        const double pi = pi_reduce_approach(total_points, num_threads);
        const auto t1 = std::chrono::steady_clock::now();
        const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        std::cout << std::format("2. Local reduce (no sync during work)\n");
        std::cout << std::format("   π ≈ {:.6f}  (error: {:.2e})  [{:.1f} ms]\n",
                                 pi, std::abs(pi - std::numbers::pi), ms);
    }

    // -------------------------------------------------------------------------
    // 3. Atomic counter (lock-free tracking)
    // -------------------------------------------------------------------------
    {
        const auto t0 = std::chrono::steady_clock::now();
        const double pi = pi_atomic_counter(total_points, num_threads);
        const auto t1 = std::chrono::steady_clock::now();
        const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        std::cout << std::format("3. Atomic counter (lock-free)\n");
        std::cout << std::format("   π ≈ {:.6f}  (error: {:.2e})  [{:.1f} ms]\n",
                                 pi, std::abs(pi - std::numbers::pi), ms);
    }

    // -------------------------------------------------------------------------
    // 4. Concurrency guidance for quant simulation
    // -------------------------------------------------------------------------
    std::cout << "\n--- Concurrency guidance ---\n";
    std::cout << "  Monte Carlo: local sums + reduce (avoid shared state).\n";
    std::cout << "  Low-latency: avoid locks on hot paths; use atomics.\n";
    std::cout << "  std::jthread joins automatically — safer than raw thread.\n";

    return 0;
}
