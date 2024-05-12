#cmake_minimum_required(VERSION 3.0)

project(SiMpLE)

add_compile_options(-std=c++17 -O3)
set(CMAKE_CXX_STANDARD 17)

# Find the required packages.
find_package(nanoflann REQUIRED)
find_package(TBB REQUIRED) 
find_package(dlib REQUIRED)
find_package(Threads REQUIRED)
find_package(yaml-cpp REQUIRED)
 
# Add an executable called simple.
add_executable(simple main.cpp ObjectiveFunction.cpp ConfigParser.cpp
                      Scan.cpp Map.cpp Register.cpp utils.cpp)

target_link_libraries(simple nanoflann::nanoflann)
target_link_libraries(simple dlib::dlib)
target_link_libraries(simple TBB::tbb)
target_link_libraries(simple yaml-cpp::yaml-cpp)
