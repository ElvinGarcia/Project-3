cmake_minimum_required(VERSION 3.22)
project(Project_3)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find the OpenMP package and its components
find_package(OpenMP REQUIRED)

add_executable(Project_3 main.cpp)

# --- This is the corrected section ---
# We manually add the flags because the OpenMP::OpenMP target failed to create.
# We need the flags for both compiling...
target_compile_options(Project_3 PRIVATE ${OpenMP_CXX_FLAGS})
# ...and linking (this is the flag that fixes the "undefined reference" error).
target_link_libraries(Project_3 PRIVATE ${OpenMP_CXX_FLAGS})
