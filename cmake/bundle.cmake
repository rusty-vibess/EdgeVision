# ---- Build bundle for target ----
if(NOT DEFINED EDGEVISION_OVERLAY_ROOT)
    message(FATAL_ERROR "EDGEVISION_OVERLAY_ROOT is not set")
endif()
if(NOT DEFINED EDGEVISION_BUNDLE_DIR)
    message(FATAL_ERROR "EDGEVISION_BUNDLE_DIR is not set")
endif()
if(NOT DEFINED EDGEVISION_BINARY)
    message(FATAL_ERROR "EDGEVISION_BINARY is not set")
endif()

# Install dir of deps
set(EDGEVISION_INSTALL_DIR "${EDGEVISION_OVERLAY_ROOT}/install")

file(MAKE_DIRECTORY "${EDGEVISION_BUNDLE_DIR}/bin")
file(MAKE_DIRECTORY "${EDGEVISION_BUNDLE_DIR}/lib")

if(EXISTS "${EDGEVISION_BINARY}")
    file(COPY "${EDGEVISION_BINARY}" DESTINATION "${EDGEVISION_BUNDLE_DIR}/bin")
    get_filename_component(_edgevision_bin_name "${EDGEVISION_BINARY}" NAME)
    set(_edgevision_bundle_bin "${EDGEVISION_BUNDLE_DIR}/bin/${_edgevision_bin_name}")
    if(UNIX AND NOT APPLE)
        file(RPATH_SET FILE "${_edgevision_bundle_bin}" NEW_RPATH "\$ORIGIN/../lib")
    endif()
else()
    message(FATAL_ERROR "EdgeVision binary not found at ${EDGEVISION_BINARY}")
endif()

if(EXISTS "${EDGEVISION_INSTALL_DIR}/lib")
    file(COPY "${EDGEVISION_INSTALL_DIR}/lib" DESTINATION "${EDGEVISION_BUNDLE_DIR}")
endif()
if(EXISTS "${EDGEVISION_INSTALL_DIR}/lib64")
    file(COPY "${EDGEVISION_INSTALL_DIR}/lib64" DESTINATION "${EDGEVISION_BUNDLE_DIR}")
endif()
