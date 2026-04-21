# @author Oliver Thomson Brown

add_executable(tests
  main.cpp
)
target_link_libraries(tests PRIVATE QuEST::QuEST Catch2::Catch2)
target_compile_features(tests PUBLIC cxx_std_20)


# DEBUG:
# trying to get MSVC to compile
target_compile_options(Catch2 PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/EHsc>
  $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
  $<$<CXX_COMPILER_ID:MSVC>:/Zm500>
)
add_compile_definitions(CATCH_CONFIG_DISABLE_STRINGIFICATION)
add_compile_definitions(CATCH_CONFIG_FAST_COMPILE)


add_subdirectory(unit)
add_subdirectory(utils)
add_subdirectory(integration)

if (ENABLE_DEPRECATED_API)
  add_subdirectory(deprecated)
endif()


# let Catch2 register all tests with CTest
catch_discover_tests(tests)
