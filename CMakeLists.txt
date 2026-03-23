cmake_minimum_required(VERSION 3.16)
project(news_sentiment_alpha_model VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(NEWS_ALPHA_BUILD_TESTS "Build News Alpha tests" ON)

find_path(EIGEN3_INCLUDE_DIR Eigen/Dense
    PATHS
        /opt/homebrew/opt/eigen/include/eigen3
        /usr/local/include/eigen3
        /opt/miniconda3/include/eigen3
)

if(NOT EIGEN3_INCLUDE_DIR)
    message(FATAL_ERROR "Eigen headers not found. Install eigen or update EIGEN3_INCLUDE_DIR.")
endif()

add_library(news_alpha_lib
    src/csv_reader.cpp
    src/data_loader.cpp
    src/feature_engineering.cpp
    src/event_study.cpp
    src/backtest.cpp
    src/reporting.cpp
    src/scaled_dataset.cpp
)

target_include_directories(news_alpha_lib PUBLIC include)
target_include_directories(news_alpha_lib SYSTEM PUBLIC ${EIGEN3_INCLUDE_DIR})

add_executable(news_alpha src/main.cpp)
target_link_libraries(news_alpha PRIVATE news_alpha_lib)

if(NEWS_ALPHA_BUILD_TESTS)
    enable_testing()
    add_executable(news_alpha_tests tests/test_pipeline.cpp)
    target_link_libraries(news_alpha_tests PRIVATE news_alpha_lib)
    target_compile_definitions(news_alpha_tests PRIVATE NEWS_ALPHA_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
    add_test(NAME news_alpha_tests COMMAND news_alpha_tests)
endif()
