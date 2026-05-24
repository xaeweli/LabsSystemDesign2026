cmake_minimum_required(VERSION 3.14)

project(recipe-manager-event-driven CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(USERVER_FEATURE_TESTSUITE OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_UTEST OFF CACHE BOOL "" FORCE)
set(USERVER_BUILD_TESTS OFF CACHE BOOL "" FORCE)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/userver/CMakeLists.txt")
    add_subdirectory(third_party/userver)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../system_design/homework_2/third_party/userver/CMakeLists.txt")
    add_subdirectory(../system_design/homework_2/third_party/userver "${CMAKE_BINARY_DIR}/third_party/userver")
else()
    find_package(userver COMPONENTS core REQUIRED)
endif()

include(FetchContent)
FetchContent_Declare(
    rabbitmq-c
    GIT_REPOSITORY https://github.com/alanxz/rabbitmq-c.git
    GIT_TAG v0.13.0
)
set(ENABLE_SSL_SUPPORT OFF CACHE BOOL "" FORCE)
set(ENABLE_PQ_SUPPORT OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(rabbitmq-c)

if(COMMAND userver_setup_environment)
    userver_setup_environment()
endif()

add_executable(${PROJECT_NAME}
    src/api_utils.cpp
    src/event_types.cpp
    src/event_producer.cpp
    src/event_consumer.cpp
    src/recipe_storage.cpp
    src/recipe_read_model.cpp
    src/handlers.cpp
    src/main.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE userver::core rabbitmq::rabbitmq)
