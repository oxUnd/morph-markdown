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
	src/base/md_table_layout.c
	src/base/md_utf8.c
	src/base/md_width.c
	src/md_highlight.c
	src/md_math_ext.c
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
option(MORPH_MARKDOWN_BUILD_KITTY "Build the Kitty terminal SDK" ON)
option(MORPH_MARKDOWN_BUILD_KITTY_DEMO "Build the Kitty terminal demo" ON)

if(MORPH_MARKDOWN_BUILD_TESTS)
	enable_testing()
endif()

set(MORPH_MATHJAX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.third_party/mathjax-c" CACHE PATH "mathjax-c source")

if(MORPH_MARKDOWN_BUILD_KITTY AND TARGET mathjax)
	add_library(morph-mathjax ALIAS mathjax)
	add_subdirectory(sdk/kitty/morph-markdown)
	if(MORPH_MARKDOWN_BUILD_KITTY_DEMO)
		add_subdirectory(demo/kitty)
	endif()
elseif(MORPH_MARKDOWN_BUILD_KITTY AND EXISTS "${MORPH_MATHJAX_DIR}/include/mathjax.h")
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(FREETYPE REQUIRED freetype2)
	pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)

	add_library(morph-mathjax STATIC
		${MORPH_MATHJAX_DIR}/src/api.c
		${MORPH_MATHJAX_DIR}/src/ast/node.c
		${MORPH_MATHJAX_DIR}/src/ast/factory.c
		${MORPH_MATHJAX_DIR}/src/ast/visitor.c
		${MORPH_MATHJAX_DIR}/src/font/font.c
		${MORPH_MATHJAX_DIR}/src/font/metrics.c
		${MORPH_MATHJAX_DIR}/src/font/stix_data.c
		${MORPH_MATHJAX_DIR}/src/render/bitmap.c
		${MORPH_MATHJAX_DIR}/src/render/draw.c
		${MORPH_MATHJAX_DIR}/src/kitty/kitty.c
		${MORPH_MATHJAX_DIR}/src/layout/box.c
		${MORPH_MATHJAX_DIR}/src/layout/math.c
		${MORPH_MATHJAX_DIR}/src/layout/scripts.c
		${MORPH_MATHJAX_DIR}/src/layout/fractions.c
		${MORPH_MATHJAX_DIR}/src/layout/radicals.c
		${MORPH_MATHJAX_DIR}/src/layout/matrix.c
		${MORPH_MATHJAX_DIR}/src/layout/spacing.c
		${MORPH_MATHJAX_DIR}/src/parser/lexer.c
		${MORPH_MATHJAX_DIR}/src/parser/parser.c
		${MORPH_MATHJAX_DIR}/src/parser/macros.c
		${MORPH_MATHJAX_DIR}/src/parser/symbols.c
		${MORPH_MATHJAX_DIR}/src/parser/environments.c
	)
	target_include_directories(morph-mathjax PUBLIC
		${MORPH_MATHJAX_DIR}/include
		${MORPH_MATHJAX_DIR}/src/third_party
		${FREETYPE_INCLUDE_DIRS}
		${HARFBUZZ_INCLUDE_DIRS}
	)
	target_link_directories(morph-mathjax PUBLIC
		${FREETYPE_LIBRARY_DIRS}
		${HARFBUZZ_LIBRARY_DIRS}
	)
	target_link_libraries(morph-mathjax PUBLIC
		${FREETYPE_LIBRARIES}
		${HARFBUZZ_LIBRARIES}
		m
	)
	if(APPLE)
		find_library(CORE_TEXT CoreText)
		find_library(CORE_GRAPHICS CoreGraphics)
		if(CORE_TEXT AND CORE_GRAPHICS)
			target_link_libraries(morph-mathjax PRIVATE
				${CORE_TEXT}
				${CORE_GRAPHICS}
			)
		endif()
	endif()

	add_subdirectory(sdk/kitty/morph-markdown)
	if(MORPH_MARKDOWN_BUILD_KITTY_DEMO)
		add_subdirectory(demo/kitty)
	endif()
endif()

if(MORPH_MARKDOWN_BUILD_TESTS)
	find_package(Python3 QUIET COMPONENTS Interpreter)

	add_executable(test_md_engine
		tests/test_md_engine.c
	)
	target_link_libraries(test_md_engine PRIVATE morph-markdown-render)

	add_test(NAME test_md_engine COMMAND test_md_engine)

	add_executable(test_md_table_layout
		tests/test_md_table_layout.c
	)
	target_link_libraries(test_md_table_layout PRIVATE morph-markdown-render)

	add_test(NAME test_md_table_layout COMMAND test_md_table_layout)

	add_executable(test_md_highlight
		tests/test_md_highlight.c
	)
	target_link_libraries(test_md_highlight PRIVATE morph-markdown-render)

	add_test(NAME test_md_highlight COMMAND test_md_highlight)

	if(Python3_Interpreter_FOUND)
		add_test(
			NAME cmark_gfm_commonmark_spec
			COMMAND ${Python3_EXECUTABLE}
				${cmark-gfm_SOURCE_DIR}/test/spec_tests.py
				-p ${cmark-gfm_BINARY_DIR}/src/cmark-gfm
				-s ${cmark-gfm_SOURCE_DIR}/test/spec.txt
		)
		add_test(
			NAME cmark_gfm_extensions_spec
			COMMAND ${Python3_EXECUTABLE}
				${cmark-gfm_SOURCE_DIR}/test/spec_tests.py
				-p ${cmark-gfm_BINARY_DIR}/src/cmark-gfm
				-s ${cmark-gfm_SOURCE_DIR}/test/extensions.txt
				--extensions "table strikethrough autolink tagfilter footnotes tasklist"
		)
	endif()
endif()
