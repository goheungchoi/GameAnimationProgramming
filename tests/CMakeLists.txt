set(TEST_NAME "InputManagerMultithreadingStressTest")

# Add source files.
file(GLOB TEST_SOURCES
  ${TEST_NAME}.cpp
  ${CMAKE_SOURCE_DIR}/tools/CircularBuffer.cpp
	${CMAKE_SOURCE_DIR}/tools/Input.cpp
	${CMAKE_SOURCE_DIR}/tools/Signal.cpp
)

add_executable(${TEST_NAME} ${TEST_SOURCES})
# Executable's include directories.
target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/tools)

if(MSVC)
  target_link_libraries(${TEST_NAME} PRIVATE glfw)
else()
  # Clang and GCC may need libstd++ and libmath
  target_link_libraries(${TEST_NAME} PRIVATE ${GLFW3_LIBRARY} stdc++ m)
endif()