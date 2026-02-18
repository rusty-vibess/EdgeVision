# Torch for Jetson targets are mostly distributed as Python wheels, so we install() from that,
# instead of a typical repo or distro
set(TORCH_SITE_PACKAGES "/opt/jetson-sysroot-overlay/site-packages/torch")

if(EXISTS "${TORCH_SITE_PACKAGES}")
    install(DIRECTORY "${TORCH_SITE_PACKAGES}/share/cmake/"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/share/cmake")
    install(DIRECTORY "${TORCH_SITE_PACKAGES}/lib/"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
    install(DIRECTORY "${TORCH_SITE_PACKAGES}/include/"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/include")
    install(DIRECTORY "${TORCH_SITE_PACKAGES}/bin/"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
else()
    message(STATUS "Torch site-packages not found at ${TORCH_SITE_PACKAGES}; skipping mirror")
endif()
