add_executable(reconstruction_contract_tests
    ReconstructionContracts.cpp
    BufferView.cpp
    TsdfTypes.cpp
    TsdfRaycaster.cpp
    HybridTypes.cpp
    GaussianResidualModel.cpp
)

target_link_libraries(reconstruction_contract_tests PRIVATE reconstruction)

add_test(NAME reconstruction_contract_tests COMMAND reconstruction_contract_tests)
register_bundle_test(reconstruction_contract_tests)
