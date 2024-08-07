set(SRCS
	HttpCacheStream.cpp HttpCacheStream.h
	Http.cpp Http.h
	Request.cpp Request.h
)

set(LIB http)
set(LIBS core io)
if (UNIX)
	find_package(CURL)

	if(CURL_FOUND)
		list(APPEND LIBS CURL::libcurl)
		message(STATUS "Found libcurl")
	else()
		message(WARNING "No libcurl found")
	endif()

	set(USE_CURL ${CURL_FOUND} CACHE BOOL "Use curl for https requests" FORCE)
endif()

engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${LIBS})

set(TEST_SRCS
	tests/RequestTest.cpp
	tests/HttpCacheStreamTest.cpp
)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} test-app json)
gtest_suite_end(tests-${LIB})
