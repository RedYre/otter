# Set the minimum version of CMake that can be used
# To find the cmake version run
# $ cmake --version
cmake_minimum_required(VERSION 3.5)

# Set the project name
project (OTR)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set (CMAKE_CXX_STANDARD 20)


file(GLOB SOURCES
     "src/BoundaryHandler.cpp"
     "src/Configurator.cpp"
     "src/LocHandler.cpp"
     "src/ArgumentParser.cpp"
     "src/Reader.cpp"
     "src/StatHandler.cpp"
     "src/TagHandler.cpp"
     "src/Writer.cpp"
     #"test/Test.cpp"
     "include/*"
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





include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
FetchContent_MakeAvailable(googletest)
enable_testing()


include_directories(include)


# Test Configurator
add_executable(
  ConfiguratorTest
  test/ConfiguratorTest.cpp
  include/Configurator.h
  include/ArgumentParser.h
  src/ArgumentParser.cpp
  src/Configurator.cpp
)
target_include_directories(ConfiguratorTest PRIVATE ${YOUR_DIRECTORY})
target_link_libraries(
  ConfiguratorTest
  gtest_main
)

# Test ArgumentOption
add_executable(
  ArgumentOptionTest
  test/ArgumentOptionTest.cpp
  include/ArgumentOption.h
)
target_include_directories(ArgumentOptionTest PRIVATE ${YOUR_DIRECTORY})
target_link_libraries(
  ArgumentOptionTest
  gtest_main
)

# Test TagHandler
add_executable(
  TagHandlerTest
  test/TagHandlerTest.cpp
  include/TagHandler.h
  src/TagHandler.cpp
  include/ArgumentParser.h
  src/ArgumentParser.cpp
  include/Configurator.h
  src/Configurator.cpp

)
target_include_directories(TagHandlerTest PRIVATE ${YOUR_DIRECTORY})
target_link_libraries(
  TagHandlerTest
  gtest_main
  ${OSMIUM_LIBRARIES}
  ${Boost_LIBRARIES}
)








# Test HandlerTest
add_executable(
  HandlerTest
  test/HandlerTest.cpp
  include/BoundaryHandler.h
  src/BoundaryHandler.cpp
  include/StatHandler.h
  src/StatHandler.cpp
  include/LocHandler.h
  src/LocHandler.cpp
  include/Configurator.h
  src/Configurator.cpp
  include/ArgumentParser.h
  src/ArgumentParser.cpp

)
target_include_directories(HandlerTest PRIVATE ${YOUR_DIRECTORY})
target_link_libraries(
  HandlerTest
  gtest_main
  ${OSMIUM_LIBRARIES}
  ${Boost_LIBRARIES}
  ${STXXL_LIBRARIES}

)




# set(INSTALL_GTEST OFF CACHE BOOL "Suppressing installation of googletest" FORCE)
# set_target_properties(gtest PROPERTIES FOLDER gtest)
# set_target_properties(gtest_main PROPERTIES FOLDER gtest)
# set_target_properties(gmock PROPERTIES FOLDER gtest)
# set_target_properties(gmock_main PROPERTIES FOLDER gtest)

# add_executable(bb)




# include_directories(include)


# include(FetchContent)
# FetchContent_Declare(
  # googletest
  # URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
# )
# enable_testing()

# add_executable(
  # test_exec
  # test/Test.cpp
# )
# target_link_libraries(
  # test_exec
  # gtest_main
  # ${OSMIUM_LIBRARIES}
  # ${Boost_LIBRARIES}
  # ${STXXL_LIBRARIES}
# )

# include(GoogleTest)
# gtest_discover_tests(test_exec)










# set(FILES_TO_TEST src/Configurator.cpp)
# set(UNIT_TESTS test/Test.cpp)
# set(MAIN_FILE test/Main.cpp)

# add_subdirectory(gtest) #Build all the gtest stuff
# include_directories(gtest/googletest/include)
# add_library(codeToTest ${FILES_TO_TEST})

# add_executable(maint ${MAIN_FILE})
# target_link_libraries(maint codeToTest)

# add_executable(unittest ${UNIT_TESTS})




# target_link_libraries(unittest gtest gtest_main ${OSMIUM_LIBRARIES} ${Boost_LIBRARIES} ${STXXL_LIBRARIES})

# Set the directories that should be included in the build command for this target
# when running g++ these will be included as -I/directory/path/
# target_include_directories(unittest
 # PRIVATE 
   # ${PROJECT_SOURCE_DIR}/include
# )
