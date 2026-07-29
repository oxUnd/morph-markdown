add_library(morph-kitty-protocol STATIC
	src/morph_kitty_protocol.c
)

add_library(MorphMarkdown::KittyProtocol ALIAS morph-kitty-protocol)

target_include_directories(morph-kitty-protocol PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
	target_compile_options(morph-kitty-protocol PRIVATE
		-Wall -Wextra -Wpedantic -Wshadow -Wconversion
	)
endif()

add_library(morph-markdown-kitty STATIC
	src/morph_markdown_kitty.c
)

add_library(MorphMarkdown::Kitty ALIAS morph-markdown-kitty)

target_include_directories(morph-markdown-kitty
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${PROJECT_SOURCE_DIR}/src
		${cmark-gfm_SOURCE_DIR}/src
		${cmark-gfm_SOURCE_DIR}/extensions
		${cmark-gfm_BINARY_DIR}/src
)

target_link_libraries(morph-markdown-kitty PUBLIC
	morph-markdown-render
	morph-mathjax
	MorphMarkdown::KittyProtocol
)

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
	target_compile_options(morph-markdown-kitty PRIVATE
		-Wall -Wextra -Wpedantic -Wshadow -Wconversion
	)
endif()

if(MORPH_MARKDOWN_BUILD_TESTS)
	add_executable(test_morph_markdown_kitty
		tests/test_morph_markdown_kitty.c
	)
	target_link_libraries(test_morph_markdown_kitty PRIVATE
		morph-markdown-kitty
	)
	target_compile_definitions(test_morph_markdown_kitty PRIVATE
		MORPH_TEST_MATH_FONT_PATH="${MORPH_MATHJAX_DIR}/fonts/STIXTwoMath-Regular.ttf"
	)
	if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
		target_compile_options(test_morph_markdown_kitty PRIVATE -UNDEBUG)
	endif()
	add_test(NAME test_morph_markdown_kitty COMMAND test_morph_markdown_kitty)
endif()
