include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(service_locator_test
    utils/service_locator/service_locator_test.cpp
)
target_link_libraries(service_locator_test PRIVATE GTest::gtest_main)

add_executable(thread_pool_test
    utils/thread_pool/thread_pool_test.cpp
)
target_link_libraries(thread_pool_test PRIVATE GTest::gtest_main tp)

add_test(NAME service_locator COMMAND service_locator_test)
add_test(NAME thread_pool COMMAND thread_pool_test)
