add_executable(morph-md-kitty-demo
	main.c
	markdown_fixtures.c
)

target_link_libraries(morph-md-kitty-demo PRIVATE
	MorphMarkdown::Kitty
)

target_compile_definitions(morph-md-kitty-demo PRIVATE
	MORPH_MATHJAX_FONT_PATH="${MORPH_MATHJAX_DIR}/fonts/STIXTwoMath-Regular.ttf"
)

if(MORPH_MARKDOWN_BUILD_TESTS)
	add_test(
		NAME morph_md_kitty_demo_smoke
		COMMAND morph-md-kitty-demo --no-delay
	)
endif()
