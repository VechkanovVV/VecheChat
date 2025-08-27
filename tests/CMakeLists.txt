include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

FetchContent_MakeAvailable(googletest)

add_executable(tests
    utils/service_locator/service_locator_test.cpp
)
target_link_libraries(tests PRIVATE  GTest::gtest_main)

add_test(NAME test COMMAND tests)
