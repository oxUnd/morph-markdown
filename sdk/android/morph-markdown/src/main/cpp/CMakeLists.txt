cmake_minimum_required(VERSION 3.22)

project(markdown_render_android_sdk C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

if(NOT ANDROID)
	message(FATAL_ERROR "This SDK CMake project is Android-only.")
endif()

include(FetchContent)

set(MORPH_MARKDOWN_RENDER_DIR
	"${CMAKE_CURRENT_SOURCE_DIR}/../../../../../.."
	CACHE PATH "markdown-render source root")
set(MORPH_MATHJAX_DIR
	"${MORPH_MARKDOWN_RENDER_DIR}/.third_party/mathjax-c"
	CACHE PATH "mathjax-c source root")
set(MORPH_MATHJAX_ANDROID_PREFIX
	"${MORPH_MARKDOWN_RENDER_DIR}/sdk/android/morph-markdown/.build/vendor-android/${ANDROID_ABI}"
	CACHE PATH "Android prefix containing freetype2 and harfbuzz")

if(NOT EXISTS "${MORPH_MATHJAX_DIR}/include/mathjax.h")
	message(FATAL_ERROR
		"Missing MathJax-C at ${MORPH_MATHJAX_DIR}. "
		"Run scripts/prepare-third-party.sh from the repository root, "
		"or pass -DMORPH_MATHJAX_DIR=/path/to/mathjax-c.")
endif()

if(NOT EXISTS "${MORPH_MATHJAX_ANDROID_PREFIX}/lib/pkgconfig/freetype2.pc"
	OR NOT EXISTS "${MORPH_MATHJAX_ANDROID_PREFIX}/lib/pkgconfig/harfbuzz.pc")
	message(FATAL_ERROR
		"Missing Android freetype/harfbuzz prefix at ${MORPH_MATHJAX_ANDROID_PREFIX}. "
		"Run sdk/android/morph-markdown/scripts/prepare-android-deps.sh, "
		"or pass -DMORPH_MATHJAX_ANDROID_PREFIX=/path/to/prefix.")
endif()

FetchContent_Declare(
	cmark-gfm
	GIT_REPOSITORY https://github.com/github/cmark-gfm.git
	GIT_TAG 0.29.0.gfm.13
)

set(CMARK_TESTS OFF CACHE BOOL "" FORCE)
set(CMARK_STATIC ON CACHE BOOL "" FORCE)
set(CMARK_SHARED OFF CACHE BOOL "" FORCE)
set(CMARK_LIB_FUZZER OFF CACHE BOOL "" FORCE)
set(CMARK_SHARED_EXTENSIONS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(cmark-gfm)

find_package(PkgConfig REQUIRED)
set(ENV{PKG_CONFIG_LIBDIR}
	"${MORPH_MATHJAX_ANDROID_PREFIX}/lib/pkgconfig:${MORPH_MATHJAX_ANDROID_PREFIX}/share/pkgconfig")
set(ENV{PKG_CONFIG_PATH}
	"${MORPH_MATHJAX_ANDROID_PREFIX}/lib/pkgconfig:${MORPH_MATHJAX_ANDROID_PREFIX}/share/pkgconfig")
set(ENV{PKG_CONFIG_SYSROOT_DIR} "")
set(PKG_CONFIG_USE_STATIC_LIBS ON)
pkg_check_modules(FREETYPE REQUIRED freetype2)
pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)
list(FILTER FREETYPE_STATIC_LDFLAGS EXCLUDE REGEX "^-lc\\+\\+$")
list(FILTER HARFBUZZ_STATIC_LDFLAGS EXCLUDE REGEX "^-lc\\+\\+$")

add_library(morph-markdown SHARED
	markdown_render_jni.c
	${MORPH_MARKDOWN_RENDER_DIR}/src/base/md_array.c
	${MORPH_MARKDOWN_RENDER_DIR}/src/base/md_buf.c
	${MORPH_MARKDOWN_RENDER_DIR}/src/base/md_hash.c
	${MORPH_MARKDOWN_RENDER_DIR}/src/base/md_strmap.c
	${MORPH_MARKDOWN_RENDER_DIR}/src/base/md_utf8.c
	${MORPH_MARKDOWN_RENDER_DIR}/src/base/md_width.c
	${MORPH_MARKDOWN_RENDER_DIR}/src/md_math_ext.c
	${MORPH_MARKDOWN_RENDER_DIR}/src/md_stream.c
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

set_target_properties(morph-markdown PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(morph-markdown PRIVATE
	${MORPH_MARKDOWN_RENDER_DIR}/include
	${MORPH_MARKDOWN_RENDER_DIR}/src
	${MORPH_MATHJAX_DIR}/include
	${MORPH_MATHJAX_DIR}/src/third_party
	${cmark-gfm_SOURCE_DIR}/src
	${cmark-gfm_SOURCE_DIR}/extensions
	${cmark-gfm_BINARY_DIR}/src
	${FREETYPE_INCLUDE_DIRS}
	${HARFBUZZ_INCLUDE_DIRS}
)

find_library(ANDROID_LOG log)

target_link_libraries(morph-markdown PRIVATE
	libcmark-gfm_static
	libcmark-gfm-extensions_static
	${FREETYPE_STATIC_LDFLAGS}
	${HARFBUZZ_STATIC_LDFLAGS}
	${ANDROID_LOG}
	c++_static
	m
	z
)

target_compile_options(morph-markdown PRIVATE
	-Wall
	-Wextra
	-Wpedantic
	-Wno-conversion
	-Wno-sign-conversion
)

target_link_options(morph-markdown PRIVATE
	"-Wl,-z,max-page-size=16384"
	"-Wl,-z,common-page-size=16384"
)
