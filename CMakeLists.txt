cmake_minimum_required(VERSION 3.22)
project(Terrain VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

FILE(GLOB SOURCE_FILES sources/*.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC "includes")
target_compile_options(${PROJECT_NAME} PRIVATE $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-O3>)

include(FetchContent)

FetchContent_Declare(
    limcore
    GIT_REPOSITORY https://github.com/ferrefire/Limcore.git
    GIT_TAG main
	GIT_SHALLOW 1
)
FetchContent_MakeAvailable("limcore")
target_link_libraries(${PROJECT_NAME} PUBLIC "limcore")