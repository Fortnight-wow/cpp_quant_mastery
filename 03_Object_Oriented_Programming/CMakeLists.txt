# Module 03 — Object-Oriented Programming: one executable per lesson file.

set(_module03_lessons
    classes_objects
    polymorphism_virtual_functions
)

foreach(_lesson IN LISTS _module03_lessons)
    add_executable(${_lesson} ${_lesson}.cpp)
    target_compile_features(${_lesson} PRIVATE cxx_std_20)
endforeach()

get_property(_existing_lessons CACHE CPP_QUANT_LESSON_TARGETS PROPERTY VALUE)
if(NOT _existing_lessons)
    set(_existing_lessons "")
endif()
list(APPEND _existing_lessons ${_module03_lessons})
set(CPP_QUANT_LESSON_TARGETS "${_existing_lessons}" CACHE INTERNAL "" FORCE)
