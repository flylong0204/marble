if (NOT CMAKE_SYSTEM_NAME STREQUAL Android)
    add_subdirectory(marble-ui)
    add_subdirectory(marble-qt)
    add_subdirectory(marble-kde)
    if( QT_QTDECLARATIVE_FOUND OR Qt5Quick_FOUND )
        add_subdirectory(marble-touch)
    endif( QT_QTDECLARATIVE_FOUND OR Qt5Quick_FOUND )
endif()

add_subdirectory(behaim)
add_subdirectory(marble-maps)
