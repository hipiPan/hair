cmake_minimum_required(VERSION 3.13)
project(hair)
set(CMAKE_CXX_STANDARD 14)

add_definitions(-DPROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE SOURCE_FILES "src/*.h" "src/*.cpp")
add_executable(hair ${SOURCE_FILES})

# spark
add_subdirectory(extern/spark EXCLUDE_FROM_ALL spark.out)
target_link_libraries(hair PUBLIC spark)

# Imath
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/extern/Imath/VS2015/x64/lib/cmake/Imath)

# alembic
add_subdirectory(extern/alembic EXCLUDE_FROM_ALL alembic.out)
target_include_directories(hair PUBLIC extern/alembic/lib/Alembic)
target_link_libraries(hair PUBLIC Alembic)