set(kritaroundcornersfilter_PART_SRCS kis_round_corners_filter_plugin.cpp kis_round_corners_filter.cpp )

add_library(kritaroundcornersfilter MODULE ${kritaroundcornersfilter_PART_SRCS})
kcoreaddons_desktop_to_json(kritaroundcornersfilter kritaroundcornersfilter.desktop)


target_link_libraries(kritaroundcornersfilter kritaui)

install(TARGETS kritaroundcornersfilter  DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES  kritaroundcornersfilter.desktop  DESTINATION ${SERVICES_INSTALL_DIR}/calligra)

