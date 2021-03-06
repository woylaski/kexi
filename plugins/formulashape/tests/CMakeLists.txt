enable_testing()

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
########### AttributeManager Test ###############

 set( TestAttributeManager_SRCS TestAttributeManager.cpp )
 kde4_add_unit_test( TestAttributeManager TESTNAME kformula-AttributeManager  ${TestAttributeManager_SRCS} )
 target_link_libraries( TestAttributeManager kformula Qt5::Test )

########### Load Test ###############

set(TestLoad_SRCS TestLoad.cpp)
kde4_add_unit_test(TestLoad TESTNAME kformula-Load  ${TestLoad_SRCS})
target_link_libraries(TestLoad kformula Qt5::Test)

########### Load and Save Test ###############

set(TestLoadAndSave_SRCS TestLoadAndSave.cpp)
kde4_add_unit_test(TestLoadAndSave TESTNAME kformula-LoadAndSave  ${TestLoadAndSave_SRCS})
target_link_libraries(TestLoadAndSave kformula Qt5::Test)

########### Layout Test ###############

#set(TestLayout_SRCS TestLayout.cpp)
#kde4_add_unit_test(TestLayout TESTNAME kformula-Layout  ${TestLayout_SRCS})
#target_link_libraries(TestLayout kformula Qt5::Test)

########### Cursor Test ###############

set(TestCursor_SRCS
TestCursor.cpp
   ../FormulaPart.cpp
   ../KoFormulaShape.cpp
   ../FormulaDocument.cpp
   ../FormulaToolWidget.cpp
   ../FormulaCommandUpdate.cpp
   ../KoFormulaShapeFactory.cpp
   ../KoFormulaTool.cpp
   ../FormulaCommand.cpp
)
if(NOT WIN32)
set(TestCursor_SRCS
    ${TestCursor_SRCS}
    ../FormulaCursor.cpp)
endif()

kde4_add_ui_files( TestCursor_SRCS
    ../FormulaToolWidget.ui
)

kde4_add_unit_test(TestCursor TESTNAME kformula-Cursor  ${TestCursor_SRCS})
target_link_libraries(TestCursor kformula KF5::KIOWidgets Qt5::Test)
