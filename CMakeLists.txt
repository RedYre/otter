# Set the minimum version of CMake that can be used
# To find the cmake version run
# $ cmake --version
cmake_minimum_required(VERSION 3.5)

# Set the project name
project (OTR)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set (CMAKE_CXX_STANDARD 20)


file(GLOB SOURCES
     "src/*.cpp"
)

set (CMAKE_CXX_FLAGS "-O3")


# include the STXXL library in my-project
add_subdirectory(stxxl)

# external tools
find_package(Osmium REQUIRED COMPONENTS io pbf)
find_package(Boost COMPONENTS iostreams REQUIRED)
# find_package(STXXL REQUIRED)



include_directories(${OSMIUM_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIR})
include_directories(${STXXL_INCLUDE_DIRS})




# Add an executable with the above sources
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${STXXL_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "-fopenmp")
add_executable(otter ${SOURCES})



target_link_libraries(otter ${OSMIUM_LIBRARIES} ${Boost_LIBRARIES} ${STXXL_LIBRARIES})

# Set the directories that should be included in the build command for this target
# when running g++ these will be included as -I/directory/path/
target_include_directories(otter
  PRIVATE 
    ${PROJECT_SOURCE_DIR}/include
)
