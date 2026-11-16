if(NOT WIN32)
    return()
endif()

include(CPackComponent)

set(
    SHOW_LYRICS_WINDOWS_VST3_BUNDLE
    "${CMAKE_CURRENT_BINARY_DIR}/ShowLyricsPlugin_artefacts/$<CONFIG>/VST3/Show Lyrics.vst3"
)
set(
    SHOW_LYRICS_VST3_SDK_DIR
    "${juce_SOURCE_DIR}/modules/juce_audio_processors_headless/format_types/VST3_SDK"
)

install(
    DIRECTORY "${SHOW_LYRICS_WINDOWS_VST3_BUNDLE}"
    DESTINATION "."
    COMPONENT VST3
)

install(
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
        "${CMAKE_CURRENT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md"
    DESTINATION "."
    COMPONENT Documentation
)

install(
    FILES "${juce_SOURCE_DIR}/LICENSE.md"
    DESTINATION "."
    RENAME "JUCE-LICENSE.md"
    COMPONENT Documentation
)

install(
    FILES "${juce_SOURCE_DIR}/JUCE.spdx.json"
    DESTINATION "."
    RENAME "JUCE-SPDX.json"
    COMPONENT Documentation
)

install(
    FILES "${SHOW_LYRICS_VST3_SDK_DIR}/LICENSE.txt"
    DESTINATION "."
    RENAME "VST3-SDK-LICENSE.txt"
    COMPONENT Documentation
)

cpack_add_component(
    VST3
    DISPLAY_NAME "Show Lyrics VST3"
    DESCRIPTION "Show Lyrics 64-bit VST3 plug-in"
    REQUIRED
)

cpack_add_component(
    Documentation
    DISPLAY_NAME "Licenses and notices"
    DESCRIPTION "Show Lyrics and third-party license information"
    REQUIRED
)

set(CPACK_GENERATOR "NSIS64")
set(CPACK_PACKAGE_NAME "Show Lyrics")
set(CPACK_PACKAGE_VENDOR "jfheinrich")
set(CPACK_PACKAGE_CONTACT "contact@jfheinrich.eu")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Timeline-based lyric display VST3 plug-in")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/jfheinrich-eu/show-lyrics")
set(SHOW_LYRICS_WINDOWS_PACKAGE_BASENAME "Show-Lyrics-${PROJECT_VERSION}-windows-x64")
set(CPACK_PACKAGE_FILE_NAME "${SHOW_LYRICS_WINDOWS_PACKAGE_BASENAME}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Show Lyrics")
set(CPACK_PACKAGE_CHECKSUM "SHA256")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_COMPONENTS_ALL VST3 Documentation)

set(CPACK_NSIS_BRANDING_TEXT "Show Lyrics by ${CPACK_PACKAGE_VENDOR}")
set(CPACK_NSIS_BRANDING_TEXT_TRIM_POSITION RIGHT)
set(
    CPACK_NSIS_INSTALLER_MUI_ICON_CODE
    "VIProductVersion ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.0
VIFileVersion ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.0
VIAddVersionKey /LANG=0 \\\"ProductName\\\" \\\"${CPACK_PACKAGE_NAME}\\\"
VIAddVersionKey /LANG=0 \\\"ProductVersion\\\" \\\"${PROJECT_VERSION}\\\"
VIAddVersionKey /LANG=0 \\\"CompanyName\\\" \\\"${CPACK_PACKAGE_VENDOR}\\\"
VIAddVersionKey /LANG=0 \\\"FileDescription\\\" \\\"${CPACK_PACKAGE_NAME} Windows Installer\\\"
VIAddVersionKey /LANG=0 \\\"FileVersion\\\" \\\"${PROJECT_VERSION}\\\"
VIAddVersionKey /LANG=0 \\\"OriginalFilename\\\" \\\"${CPACK_PACKAGE_FILE_NAME}.exe\\\""
)

set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
set(CPACK_NSIS_DISPLAY_NAME "Show Lyrics ${PROJECT_VERSION}")
set(CPACK_NSIS_PACKAGE_NAME "Show Lyrics ${PROJECT_VERSION}")
set(CPACK_NSIS_HELP_LINK "https://github.com/jfheinrich-eu/show-lyrics/issues")
set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/jfheinrich-eu/show-lyrics")
set(CPACK_NSIS_CONTACT "contact@jfheinrich.eu")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)
set(CPACK_NSIS_VST3_INSTALL_DIRECTORY "$COMMONFILES64\\\\VST3")

file(
    GENERATE
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ShowLyricsPackageName.txt"
    CONTENT "${SHOW_LYRICS_WINDOWS_PACKAGE_BASENAME}"
)

include(CPack)
