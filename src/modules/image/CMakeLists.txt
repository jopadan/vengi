set(SRCS
	external/jo_mpeg.cpp external/jo_mpeg.h

	Image.cpp Image.h
	AVI.cpp AVI.h
	CaptureTool.cpp CaptureTool.h
)
set(LIB image)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES io util stb_image)
if (NOT MSVC)
	target_compile_options(${LIB} PRIVATE -Wno-unused-function -Wno-missing-field-initializers)
endif()

# TODO: not included as local lib yet
if (NOT USE_LIBS_FORCE_LOCAL)
	find_package(JPEG)
	if (JPEG_FOUND)
		target_compile_definitions(${LIB} PRIVATE -DUSE_LIBJPEG)
		target_link_libraries(${LIB} PRIVATE JPEG::JPEG)
	endif()
else()
	set(JPEG_FOUND 0)
endif()
set(USE_LIBJPEG ${JPEG_FOUND} CACHE BOOL "Use libjpeg for jpeg saving" FORCE)

set(TEST_SRCS
	tests/ImageTest.cpp
	tests/AVITest.cpp
	tests/MPEG2Test.cpp
)

set(TEST_FILES
	tests/test-palette-in.png
)
gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})
