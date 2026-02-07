cmake_minimum_required(VERSION 3.20)
project("minecraft_clone")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(program_name ${CMAKE_PROJECT_NAME})

file(GLOB_RECURSE src_files CONFIGURE_DEPENDS src/*.cpp)

set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")
set(compile_options
  "$<${gcc_like_cxx}:-pedantic-errors;-Wall;-Weffc++;-Wextra;-Wsign-conversion;-Wconversion>"
  "$<${msvc_cxx}:/permissive-;/W4;/WX>"
)

include(FetchContent)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
  GIT_SHALLOW TRUE
)

FetchContent_Declare(
  glm 
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.3
  GIT_SHALLOW TRUE
)

FetchContent_Declare(
  unordered_dense
  GIT_REPOSITORY https://github.com/martinus/unordered_dense.git
  GIT_TAG v4.8.1
  GIT_SHALLOW TRUE
)

FetchContent_Declare(
  fast_noise_2
  GIT_REPOSITORY https://github.com/Auburn/FastNoise2.git
  GIT_TAG v1.0.1
  GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glfw glm unordered_dense fast_noise_2)

add_subdirectory(external/glad)
add_subdirectory(external/stb_image)

add_executable(${program_name} ${src_files})
target_include_directories(${program_name} PRIVATE include)
target_compile_options(${program_name} PRIVATE ${compile_options})
target_link_libraries(${program_name} PRIVATE glfw glm::glm glad unordered_dense stb_image FastNoise)
