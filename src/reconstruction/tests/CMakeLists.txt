add_executable(reconstruction_contract_tests
    interfaces/GaussianResidualModel.cpp
    interfaces/ReconstructionPipeline.cpp
    interfaces/TsdfRaycaster.cpp
    reconstruction/ReconstructionContracts.cpp
    reconstruction/ReconstructionPipeline.cpp
    reconstruction/TorchGaussianResidualModel.cpp
    types/BufferView.cpp
    types/HybridTypes.cpp
    types/OptimizationObservationSet.cpp
    types/ReconstructionObservation.cpp
    types/ReconstructionPipelineConfig.cpp
    types/TorchGaussianBackendConfig.cpp
    types/TsdfTypes.cpp
)

target_link_libraries(reconstruction_contract_tests PRIVATE reconstruction)

add_test(
    NAME reconstruction_contract_tests
    COMMAND reconstruction_contract_tests
)
register_bundle_test(reconstruction_contract_tests)
