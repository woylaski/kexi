add_definitions(-DTRANSLATION_DOMAIN=\"krossmodulesheets\")

include_directories( 
        ${CMAKE_SOURCE_DIR}/sheets
        ${CMAKE_SOURCE_DIR}/sheets/commands
        ${CMAKE_SOURCE_DIR}/sheets/ui
	${KOTEXT_INCLUDES} 
	${KOMAIN_INCLUDES} 
	${KOKROSS_INCLUDES}
	)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ${KDE4_ENABLE_EXCEPTIONS}")

add_subdirectory( scripts )

# the Sheets Scripting Plugin + Kross module
set(krossmodulesheets_PART_SRCS ScriptingModule.cpp ScriptingFunction.cpp ScriptingWidgets.cpp ScriptingPart.cpp ScriptingListener.cpp)

qt5_wrap_cpp(krossmodulesheets_PART_SRCS ScriptingReader.h ScriptingWriter.h)
add_library(krossmodulesheets MODULE ${krossmodulesheets_PART_SRCS})

target_link_libraries(krossmodulesheets   calligrasheetscommon kokross)
install(TARGETS krossmodulesheets DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})

########### install files ###############

install( FILES krossmodulesheets.desktop  DESTINATION ${SERVICES_INSTALL_DIR}/calligra)
install( FILES scripting.rc  DESTINATION ${DATA_INSTALL_DIR}/sheets/viewplugins)
install( FILES sheetsscripting.desktop  DESTINATION ${SERVICES_INSTALL_DIR}/calligra)


