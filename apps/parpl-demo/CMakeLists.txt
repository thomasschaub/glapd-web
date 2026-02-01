file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cpp)
add_executable(parpl-demo ${srcs})
target_link_libraries(parpl-demo PRIVATE parpl)