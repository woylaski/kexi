set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories(${KOTEXT_INCLUDES} ${KDE4_INCLUDES}
        ${CMAKE_SOURCE_DIR}/libs/flake/tests
        ${CMAKE_BINARY_DIR}/libs/main
        ${CMAKE_SOURCE_DIR}/libs/kotext/tests
        ${CMAKE_SOURCE_DIR}/words/part
)

add_definitions(-DFILES_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/")
add_definitions(-DFILES_OUTPUT_DIR="${CMAKE_CURRENT_BINARY_DIR}/")

########### next target ###############

#set(TestPageManager_test_SRCS TestPageManager.cpp)
#kde4_add_unit_test(TestPageManager TESTNAME words-part-TestPageManager  ${TestPageManager_test_SRCS})
#target_link_libraries(TestPageManager wordsprivate Qt5::Test)

########### next target ###############

set(TestClipToPage_test_SRCS TestClipToPage.cpp)
kde4_add_unit_test(TestClipToPage TESTNAME words-part-TestClipToPage  ${TestClipToPage_test_SRCS})
target_link_libraries(TestClipToPage wordsprivate Qt5::Test)

########### next target ###############

#set(TestViewMode_test_SRCS TestViewMode.cpp)
#kde4_add_unit_test(TestViewMode TESTNAME words-part-TestViewMode  ${TestViewMode_test_SRCS})
#target_link_libraries(TestViewMode wordsprivate Qt5::Test)

########### next target ###############

#set(TestPageTextInfo_test_SRCS TestPageTextInfo.cpp)
#kde4_add_unit_test(TestPageTextInfo TESTNAME words-part-TestPageTextInfo  ${TestPageTextInfo_test_SRCS})
#target_link_libraries(TestPageTextInfo wordsprivate Qt5::Test)

########### next target ###############

#set(TestFrameLayout_test_SRCS TestFrameLayout.cpp)
#kde4_add_unit_test(TestFrameLayout TESTNAME words-part-frame-TestFrameLayout  ${TestFrameLayout_test_SRCS})
#target_link_libraries(TestFrameLayout wordsprivate Qt5::Test)

########### next target ###############

#set(TestTextFrameSorting_test_SRCS TestTextFrameSorting.cpp)
#kde4_add_unit_test(TestTextFrameSorting TESTNAME words-part-frame-TestTextFrameSorting  ${TestTextFrameSorting_test_SRCS})
#target_link_libraries(TestTextFrameSorting wordsprivate Qt5::Test)

########### next target ###############
#set(TestTextFrameManagement_test_SRCS TestTextFrameManagement.cpp)
#kde4_add_unit_test(TestTextFrameManagement TESTNAME words-part-frame-TestTextFrameManagement  ${TestTextFrameManagement_test_SRCS})
#target_link_libraries(TestTextFrameManagement wordsprivate Qt5::Test)

########### next target ###############

if( SHOULD_BUILD_FEATURE_RDF )
#  set(TestRdf_SRCS TestRdf.cpp )
#  kde4_add_unit_test(TestRdf TESTNAME words-part-TestRdf ${TestRdf_SRCS})
#  target_link_libraries(TestRdf wordsprivate Qt5::Test)
endif()


