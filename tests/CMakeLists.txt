# @author Oliver Thomson Brown

# DEBUG:
# trying to get MSVC to compile
add_compile_definitions(CATCH_CONFIG_WINDOWS_SEH)

add_executable(tests
  main.cpp
)
target_link_libraries(tests PRIVATE QuEST::QuEST Catch2::Catch2)
target_compile_features(tests PUBLIC cxx_std_20)


target_compile_options(tests PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/EHsc>
)


add_subdirectory(unit)
add_subdirectory(utils)
add_subdirectory(integration)

if (ENABLE_DEPRECATED_API)
  add_subdirectory(deprecated)
endif()


# let Catch2 register all tests with CTest
catch_discover_tests(tests)
