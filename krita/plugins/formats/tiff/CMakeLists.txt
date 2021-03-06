add_subdirectory(tests)

set(libkritatiffconverter_LIB_SRCS
    kis_tiff_converter.cc
    kis_tiff_writer_visitor.cpp
    kis_tiff_reader.cc
    kis_tiff_ycbcr_reader.cc
    kis_buffer_stream.cc
    )

########## next target ###############

set(kritatiffimport_PART_SRCS
    ${libkritatiffconverter_LIB_SRCS}
    kis_tiff_import.cc
    )


add_library(kritatiffimport MODULE ${kritatiffimport_PART_SRCS})
kcoreaddons_desktop_to_json(kritatiffimport krita_tiff_import.desktop)


target_link_libraries(kritatiffimport kritaui ${KDE4_KIO_LIBS} ${TIFF_LIBRARIES})

install(TARGETS kritatiffimport  DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})


########### next target ###############

set(kritatiffexport_PART_SRCS
    ${libkritatiffconverter_LIB_SRCS}
    kis_tiff_export.cc
    kis_dlg_options_tiff.cpp
    )

ki18n_wrap_ui(kritatiffexport_PART_SRCS kis_wdg_options_tiff.ui )

add_library(kritatiffexport MODULE ${kritatiffexport_PART_SRCS})
kcoreaddons_desktop_to_json(kritatiffexport krita_tiff_export.desktop)



target_link_libraries(kritatiffexport kritaui ${KDE4_KIO_LIBS} ${TIFF_LIBRARIES})

install(TARGETS kritatiffexport  DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})


########### next target ###############


########### install files ###############

install( FILES  krita_tiff_export.desktop krita_tiff_import.desktop  DESTINATION ${SERVICES_INSTALL_DIR}/calligra)
install( PROGRAMS  krita_tiff.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR})

