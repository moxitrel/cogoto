cmake_minimum_required(VERSION 3.13)

project(cogo_test)

if (PROJECT_NAME STREQUAL CMAKE_PROJECT_NAME)
    include(CTest)
    if (BUILD_TESTING)
        include(FindGTest)
        include(GoogleTest)

        # yield_case
        add_executable(yield_case_test)
        target_sources(yield_case_test
                PRIVATE yield_test.cpp)
        target_compile_features(yield_case_test
                PRIVATE cxx_std_11)
        target_compile_definitions(yield_case_test
                PRIVATE COGO_CASE)
        target_link_libraries(yield_case_test
                PRIVATE GTest::GTest GTest::Main)

        # yield_label_value
        add_executable(yield_label_value_test)
        target_sources(yield_label_value_test
                PRIVATE yield_test.cpp)
        target_compile_features(yield_label_value_test
                PRIVATE cxx_std_11)
        target_compile_definitions(yield_case_test
                PRIVATE COGO_LABEL_VALUE)
        target_link_libraries(yield_label_value_test
                PRIVATE GTest::GTest GTest::Main)

        # co
        add_executable(co_test)
        target_sources(co_test
                PRIVATE co_test.cpp)
        target_compile_features(co_test
                PRIVATE cxx_std_11)
        target_link_libraries(co_test
                PRIVATE GTest::GTest GTest::Main)

        # co_fs
        add_executable(co_fs_test)
        target_sources(co_fs_test
                PRIVATE co_fs_test.cpp)
        target_compile_features(co_fs_test
                PRIVATE cxx_std_11)
        target_link_libraries(co_fs_test
                PRIVATE GTest::GTest GTest::Main)

        gtest_discover_tests(yield_case_test)
        gtest_discover_tests(yield_label_value_test)
        gtest_discover_tests(co_test)
        gtest_discover_tests(co_fs_test)
    endif ()
endif ()
