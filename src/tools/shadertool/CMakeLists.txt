project(shadertool)

set(SHARED_SRCS
	Generator.cpp Generator.h
	Types.cpp Types.h
	Util.cpp Util.h
	TokenIterator.h
	Parser.cpp Parser.h
)

set(SRCS
	ShaderTool.h ShaderTool.cpp
	${SHARED_SRCS}
)
engine_add_build_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} NOINSTALL)
set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD FALSE)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES app video simplecpp)

set(TEST_FILES tests/testshader.glsl)

set(TEST_SRCS
	${SHARED_SRCS}
	tests/ShaderToolTest.cpp
)

gtest_suite_begin(tests-${PROJECT_NAME} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${PROJECT_NAME} ${TEST_SRCS})
gtest_suite_files(tests-${PROJECT_NAME} ${TEST_FILES})
gtest_suite_deps(tests-${PROJECT_NAME} test-app video simplecpp)
gtest_suite_end(tests-${PROJECT_NAME})
