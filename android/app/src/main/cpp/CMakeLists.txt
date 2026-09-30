cmake_minimum_required(VERSION 3.22.1)
project(mk64_android LANGUAGES C CXX)

# Android native runtime bridge. The MK64 decomp/platform sources are added
# incrementally as libultra dependencies are replaced by Android/SDL equivalents.
add_library(mk64_android SHARED native_bridge.cpp)
find_library(log-lib log)
target_link_libraries(mk64_android ${log-lib})
