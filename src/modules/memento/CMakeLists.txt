set(SRCS
	MementoHandler.h MementoHandler.cpp
)

set(LIB memento)
set(DEPENDENCIES scenegraph)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${DEPENDENCIES})

set(TEST_SRCS
	tests/MementoHandlerTest.cpp
)
gtest_suite_deps(tests ${LIB} test-app)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})
