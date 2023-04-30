cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)

project(Noise22)

add_executable(Batched batched_podc_22.cc)
add_executable(Noisy noisy_podc_22.cc)
