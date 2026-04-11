cmake_minimum_required(VERSION 3.16)
project(oldflight C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

file(GLOB OLDLIGHT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_executable(oldflight ${OLDLIGHT_SOURCES})
target_include_directories(oldflight PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(oldflight PRIVATE $<$<PLATFORM_ID:Windows>:W32>)

target_compile_options(oldflight PRIVATE
  $<$<C_COMPILER_ID:AppleClang,Clang,GNU>:-Wno-deprecated -Wno-visibility -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -Wno-comment -Wno-static-in-inline>
)

find_package(OpenGL REQUIRED)
target_link_libraries(oldflight PRIVATE OpenGL::GL)

if(APPLE)
  find_library(GLUT_FRAMEWORK GLUT REQUIRED)
  target_link_libraries(oldflight PRIVATE ${GLUT_FRAMEWORK} m)
elseif(WIN32)
  find_package(GLUT REQUIRED)
  if(GLUT_INCLUDE_DIR)
    target_include_directories(oldflight PRIVATE ${GLUT_INCLUDE_DIR})
  endif()
  target_link_libraries(oldflight PRIVATE ${GLUT_LIBRARIES} opengl32 glu32 ws2_32 winmm)
else()
  find_package(GLUT REQUIRED)
  if(GLUT_INCLUDE_DIR)
    target_include_directories(oldflight PRIVATE ${GLUT_INCLUDE_DIR})
  endif()
  target_link_libraries(oldflight PRIVATE ${GLUT_LIBRARIES} GLU m)
endif()
