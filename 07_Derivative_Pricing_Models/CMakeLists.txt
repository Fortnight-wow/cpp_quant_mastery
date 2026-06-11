# Module 07 — Derivative Pricing Models: one executable per lesson file.

set(_module07_lessons
    black_scholes
    binomial_tree
    monte_carlo
)

foreach(_lesson IN LISTS _module07_lessons)
    add_executable(${_lesson} ${_lesson}.cpp)
    target_compile_features(${_lesson} PRIVATE cxx_std_20)
endforeach()

get_property(_existing_lessons CACHE CPP_QUANT_LESSON_TARGETS PROPERTY VALUE)
if(NOT _existing_lessons)
    set(_existing_lessons "")
endif()
list(APPEND _existing_lessons ${_module07_lessons})
set(CPP_QUANT_LESSON_TARGETS "${_existing_lessons}" CACHE INTERNAL "" FORCE)
