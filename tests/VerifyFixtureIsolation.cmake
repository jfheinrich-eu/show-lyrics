# SPDX-License-Identifier: AGPL-3.0-only

if(NOT DEFINED PLUGIN_BINARY OR NOT EXISTS "${PLUGIN_BINARY}")
    message(FATAL_ERROR "The VST3 binary does not exist: ${PLUGIN_BINARY}")
endif()

file(READ "${PLUGIN_BINARY}" plugin_hex HEX)
string(TOLOWER "${plugin_hex}" plugin_hex)
string(HEX "Erste Strophe" fixture_text_hex)
string(TOLOWER "${fixture_text_hex}" fixture_text_hex)
string(FIND "${plugin_hex}" "${fixture_text_hex}" fixture_position)

if(EXPECT_FIXTURE AND fixture_position EQUAL -1)
    message(FATAL_ERROR "The enabled Debug VST3 does not contain the Cubase integration fixture.")
endif()

if(NOT EXPECT_FIXTURE AND NOT fixture_position EQUAL -1)
    message(FATAL_ERROR "The Cubase integration fixture leaked into a VST3 where it must be disabled.")
endif()

message(STATUS "Cubase integration fixture isolation verified for ${PLUGIN_BINARY}")
