cmake_minimum_required(VERSION 3.20)

project(morph-markdown-render C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

include(FetchContent)

FetchContent_Declare(
	cmark-gfm
	GIT_REPOSITORY https://github.com/github/cmark-gfm.git
	GIT_TAG        0.29.0.gfm.13
)

set(CMARK_TESTS OFF CACHE BOOL "" FORCE)
set(CMARK_STATIC ON CACHE BOOL "" FORCE)
set(CMARK_SHARED OFF CACHE BOOL "" FORCE)
set(CMARK_LIB_FUZZER OFF CACHE BOOL "" FORCE)
set(CMARK_SHARED_EXTENSIONS OFF CACHE BOOL "" FORCE)
set(CMARK_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(cmark-gfm)

add_library(morph-markdown-render STATIC
	src/base/md_array.c
	src/base/md_buf.c
	src/base/md_hash.c
	src/base/md_strmap.c
	src/base/md_utf8.c
	src/md_stream.c
)

target_include_directories(morph-markdown-render PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${cmark-gfm_SOURCE_DIR}/src
	${cmark-gfm_SOURCE_DIR}/extensions
	${cmark-gfm_BINARY_DIR}/src
)

target_link_libraries(morph-markdown-render PUBLIC
	libcmark-gfm_static
	libcmark-gfm-extensions_static
)

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
	target_compile_options(morph-markdown-render PRIVATE
		-Wall -Wextra -Wpedantic -Wshadow -Wconversion
	)
endif()

option(MORPH_MARKDOWN_BUILD_TESTS "Build markdown-render tests" ON)

if(MORPH_MARKDOWN_BUILD_TESTS)
	enable_testing()

	add_executable(test_md_stream
		tests/test_md_stream.c
	)
	target_link_libraries(test_md_stream PRIVATE morph-markdown-render)

	add_test(NAME test_md_stream COMMAND test_md_stream)
endif()
