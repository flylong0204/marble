SET (TARGET svg2pnt)
PROJECT (${TARGET})

include_directories(
 ${CMAKE_CURRENT_SOURCE_DIR}
 ${CMAKE_CURRENT_BINARY_DIR}
 ${QT_INCLUDE_DIR}
)

set( ${TARGET}_SRC svg2pnt.cpp svgxmlhandler.cpp )
add_executable( ${TARGET} ${${TARGET}_SRC} )

target_link_libraries( ${TARGET}  ${Qt5Core_LIBRARIES} ${Qt5Xml_LIBRARIES} )
