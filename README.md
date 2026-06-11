# C++ Quant Mastery

A production-grade, module-by-module learning repository that takes you from **Hello World** to **derivative pricing**, **numerical methods**, and **low-latency trading systems** using modern **C++20**.

Each source file is a self-contained lesson: a concept block comment at the top, idiomatic C++20 code, and a `main()` that runs a quantitative finance example and prints results to the console.

---

## Prerequisites

| Requirement | Minimum version | Notes |
|-------------|-----------------|-------|
| **CMake**   | 3.20+           | Configures and builds all module executables |
| **Compiler**| GCC 11+, Clang 14+, or MSVC 19.29+ | Must support C++20 (`concepts`, `ranges`, coroutines optional) |
| **OS**      | Linux, macOS, or Windows | Tested with native toolchains |

Optional but recommended for quant work:

- **Eigen** or similar (not required here—we implement linear algebra from scratch in Module 06)
- Familiarity with calculus, probability, and basic fixed-income/options terminology

---

## Quick Start

```bash
# Clone or navigate to the repository root
cd cpp-quant-mastery

# Configure (Release recommended for performance benchmarks)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Build all available module targets
cmake --build build

# Run a specific lesson (example once Module 01 is generated)
./build/01_Basics/hello_world
```

As modules are added, CMake discovers each subdirectory automatically. Targets are named after their source file (e.g., `hello_world`, `black_scholes`).

---

## Curriculum Overview

The repository follows a deliberate progression: language foundations → memory & OOP → STL → advanced C++ → numerics → pricing models → market microstructure.

```
┌─────────────────────────────────────────────────────────────────────────┐
│  Language & Foundations (Modules 01–05)                                 │
│  Syntax → Memory → OOP → STL → Templates / Concurrency                  │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  Quant Core (Modules 06–07)                                             │
│  Linear algebra → Root finding / IV → Optimization → Trees / BS / MC    │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  Systems (Module 08)                                                    │
│  Limit order book → Matching engine simulation                          │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## Module Guide

### Module 01 — Basics (`01_Basics/`)

**Goal:** C++ syntax and control flow with quant-flavored examples.

| File | Topics |
|------|--------|
| `hello_world.cpp` | Build system integration, `iostream`, first executable |
| `data_types.cpp` | `int`, `double`, `bool`, precision, financial literals |
| `control_flow.cpp` | `if`/`switch`, loops, simple P&L scenarios |

**Outcome:** Comfortable compiling and running C++ programs; understand why `double` is the default for pricing.

---

### Module 02 — Memory and Pointers (`02_Memory_and_Pointers/`)

**Goal:** RAII, ownership, and why raw pointers are rare in modern quant code.

| File | Topics |
|------|--------|
| `raw_pointers.cpp` | Stack vs heap, `new`/`delete` (legacy patterns only) |
| `smart_pointers.cpp` | `std::unique_ptr`, `std::shared_ptr`, ownership models |

**Outcome:** Write leak-free code; model instrument ownership safely.

---

### Module 03 — Object-Oriented Programming (`03_Object_Oriented_Programming/`)

**Goal:** Polymorphic pricing interfaces—the backbone of extensible libraries.

| File | Topics |
|------|--------|
| `classes_objects.cpp` | Encapsulation, constructors, `Instrument` / `Option` sketches |
| `polymorphism_virtual_functions.cpp` | Abstract base classes, virtual destructors, strategy pattern |

**Outcome:** Design a pluggable `Pricer` hierarchy without virtual dispatch overhead surprises.

---

### Module 04 — Standard Template Library (`04_Standard_Template_Library/`)

**Goal:** Containers and algorithms used daily in production systems.

| File | Topics |
|------|--------|
| `vectors_and_maps.cpp` | `std::vector`, `std::map`/`std::unordered_map`, tick storage |
| `algorithms.cpp` | `std::sort`, `std::accumulate`, `std::ranges` views |

**Outcome:** Choose the right container for time-series vs lookup tables.

---

### Module 05 — Advanced C++ (`05_Advanced_CPP/`)

**Goal:** Generic, concurrent code suitable for simulation and low-latency paths.

| File | Topics |
|------|--------|
| `templates_and_metaprogramming.cpp` | Function/class templates, `concepts`, compile-time checks |
| `multithreading_concurrency.cpp` | `std::thread`, mutexes, atomics, Monte Carlo parallelism preview |

**Outcome:** Write type-safe generic numerics; avoid data races in parallel MC.

---

### Module 06 — Numerical Methods and Linear Algebra (`06_Numerical_Methods_and_Linear_Algebra/`)

**Goal:** Mathematical machinery behind calibration and risk.

| File | Topics |
|------|--------|
| `matrix_operations.cpp` | Vectors, matrices, multiplication, Cholesky preview |
| `root_finding.cpp` | Newton–Raphson; implied volatility inversion |
| `optimization_basics.cpp` | Gradient descent; least-squares calibration sketch |

**Key math:** Linear systems \(A\mathbf{x} = \mathbf{b}\); root of \(f(\sigma) = C_{\mathrm{BS}}(\sigma) - C_{\mathrm{mkt}} = 0\).

**Outcome:** Implement IV solvers and simple calibrators without external libraries.

---

### Module 07 — Derivative Pricing Models (`07_Derivative_Pricing_Models/`)

**Goal:** From discrete trees to closed form and simulation.

| File | Topics |
|------|--------|
| `binomial_tree.cpp` | Cox–Ross–Rubinstein tree; European & American options |
| `black_scholes.cpp` | Black–Scholes–Merton PDE → closed form; Delta, Gamma, Vega, Theta, Rho |
| `monte_carlo.cpp` | GBM: \(dS_t = \mu S_t\,dt + \sigma S_t\,dW_t\); Euler–Maruyama; antithetic variates |

**Key math:**

- **Black–Scholes PDE:** \(\frac{\partial V}{\partial t} + \frac{1}{2}\sigma^2 S^2 \frac{\partial^2 V}{\partial S^2} + rS\frac{\partial V}{\partial S} - rV = 0\)
- **Risk-neutral GBM:** \(S_T = S_0 \exp\left((r - \frac{1}{2}\sigma^2)T + \sigma\sqrt{T}\,Z\right)\)

**Outcome:** Price and hedge vanilla options three ways; understand bias vs variance in MC.

---

### Module 08 — Trading Systems Simulation (`08_Trading_Systems_Simulation/`)

**Goal:** Event-driven market structure and matching logic.

| File | Topics |
|------|--------|
| `order_book.cpp` | Price-time priority; bids/asks; `std::map` / flat maps for LOB |
| `matching_engine.cpp` | Limit/market orders; partial fills; trade prints |

**Outcome:** Simulate a minimal exchange stack suitable for latency and correctness testing.

---

## Coding Standards

Every `.cpp` / `.hpp` file in this repository follows these rules:

1. **Concept header comment** — Explains the financial or mathematical idea before any code (e.g., the BS PDE, CRR tree recurrence, or order-book invariants).
2. **C++20** — Prefer `concepts`, `ranges`, smart pointers, and structured bindings; raw pointers only when teaching legacy code or documented micro-optimizations.
3. **Executable lessons** — Each file includes `main()` with a concrete numeric example (strike, spot, rate, vol) and printed output.
4. **Numerical type** — Use `double` for financial calculations; include `<cmath>` for `std::exp`, `std::erf`, etc.
5. **Performance awareness** — Avoid unnecessary allocations in hot paths (Modules 07–08); comment on complexity where relevant.

---

## Build System Design

The root `CMakeLists.txt`:

- Sets **C++20** as the required standard (`CMAKE_CXX_STANDARD 20`).
- Applies strict warnings (`-Wall -Wextra -Wpedantic` on GCC/Clang).
- **Conditionally** adds each module subdirectory when its `CMakeLists.txt` exists—so the project configures cleanly as modules are generated incrementally.

Each module's `CMakeLists.txt` (added with that module) registers one executable per `.cpp` lesson.

---

## Suggested Learning Path

| Week | Focus | Modules |
|------|-------|---------|
| 1–2 | Language fluency | 01–03 |
| 3 | STL & modern C++ | 04–05 |
| 4–5 | Numerics & IV | 06 |
| 6–7 | Pricing stack | 07 |
| 8 | Market simulation | 08 |

Re-run examples with your own parameters. Modify vol, rates, and tree steps to build intuition for stability and convergence.

---

## Mathematical Notation Reference

| Symbol | Meaning |
|--------|---------|
| \(S\) | Spot price of underlying |
| \(K\) | Strike price |
| \(T\) | Time to maturity (years) |
| \(r\) | Risk-free rate (continuously compounded) |
| \(\sigma\) | Volatility |
| \(N(\cdot)\) | Standard normal CDF |
| \(\Delta, \Gamma, \Theta, \nu\) (Vega), \(\rho\) | Option Greeks |

---

## License

Educational use. Adapt and extend for interviews, coursework, or internal training.

---

## Contributing

When adding a new lesson:

1. Place it in the appropriate module directory.
2. Follow the file header and `main()` conventions above.
3. Register the target in that module's `CMakeLists.txt`.
4. Verify `cmake --build build` succeeds in Release mode.

---

*Built for quantitative developers who want C++ that is correct, fast, and readable.*
