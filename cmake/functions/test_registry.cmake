# ---- Test Registry ----
function(register_bundle_test target_name)
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "register_bundle_test: target '${target_name}' does not exist")
    endif()

    # Appends target to GLOBAL PROPERTY EDGEVISION_TEST_TARGETS
    get_property(_tests GLOBAL PROPERTY EDGEVISION_TEST_TARGETS)
    if(NOT _tests)
        set(_tests "")
    endif()

    list(APPEND _tests ${target_name})
    list(REMOVE_DUPLICATES _tests)
    set_property(GLOBAL PROPERTY EDGEVISION_TEST_TARGETS "${_tests}")
endfunction()

function(collect_test_registry)
    get_property(_edgevision_test_targets GLOBAL PROPERTY EDGEVISION_TEST_TARGETS)

    set(_edgevision_test_binaries "")
    foreach(_test_target IN LISTS _edgevision_test_targets)
        list(APPEND _edgevision_test_binaries "$<TARGET_FILE:${_test_target}>")
    endforeach()

    list(JOIN _edgevision_test_binaries ";" _edgevision_test_binaries)

    set(EDGEVISION_TEST_TARGETS "${_edgevision_test_targets}" CACHE INTERNAL "Test targets to bundle")
    set(EDGEVISION_TEST_BINARIES "${_edgevision_test_binaries}" CACHE INTERNAL "Test binaries to bundle")
endfunction()
