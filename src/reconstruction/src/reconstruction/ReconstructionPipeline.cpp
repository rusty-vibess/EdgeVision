#include "reconstruction/ReconstructionPipeline.hpp"

#include <utility>

ReconstructionPipeline::ReconstructionPipeline(
    ITsdfRaycaster& tsdfRaycaster,
    IGaussianResidualModel& gaussianResidualModel,
    const ReconstructionPipelineConfig& config
)
    : m_tsdfRaycaster(tsdfRaycaster),
      m_gaussianResidualModel(gaussianResidualModel),
      m_config(config) {}

ReconstructionPipeline::~ReconstructionPipeline() = default;

ReconstructionObservation ReconstructionPipeline::prepareObservation(
    const ReconstructionObservation& observation
) const {
    ReconstructionObservation preparedObservation = observation;
    if (!preparedObservation.hasRaycast) {
        preparedObservation.raycast =
            m_tsdfRaycaster.raycast(preparedObservation.frame, preparedObservation.useCameraDepth);
        preparedObservation.hasRaycast = true;
    }

    return preparedObservation;
}

OptimizationObservationSet ReconstructionPipeline::prepareObservationSet(
    const OptimizationObservationSet& observationSet
) const {
    OptimizationObservationSet preparedObservationSet = observationSet;
    preparedObservationSet.includeCurrentObservationInOptimization =
        m_config.includeCurrentObservationInOptimization;

    if (preparedObservationSet.hasCurrentObservation) {
        preparedObservationSet.currentObservation =
            prepareObservation(preparedObservationSet.currentObservation);
    }

    for (ReconstructionObservation& localObservation : preparedObservationSet.localObservations) {
        localObservation = prepareObservation(localObservation);
    }

    for (ReconstructionObservation& keyframeObservation :
         preparedObservationSet.keyframeObservations) {
        keyframeObservation = prepareObservation(keyframeObservation);
    }

    if (!m_config.optimizeWithKeyframes) {
        preparedObservationSet.keyframeObservations.clear();
    }

    return preparedObservationSet;
}

void ReconstructionPipeline::seedCurrentObservation(const ReconstructionObservation& observation) {
    const ReconstructionObservation preparedObservation = prepareObservation(observation);
    m_gaussianResidualModel.seedFromResidual(preparedObservation, m_config.residualSeedConfig);
}

float ReconstructionPipeline::optimizeObservationSet(
    const OptimizationObservationSet& observationSet
) {
    const OptimizationObservationSet preparedObservationSet =
        prepareObservationSet(observationSet);
    return m_gaussianResidualModel.optimizeObservationSet(preparedObservationSet);
}

float ReconstructionPipeline::runOptimizationCycle(
    const OptimizationObservationSet& observationSet
) {
    const OptimizationObservationSet preparedObservationSet =
        prepareObservationSet(observationSet);
    if (preparedObservationSet.hasCurrentObservation) {
        m_gaussianResidualModel.seedFromResidual(
            preparedObservationSet.currentObservation, m_config.residualSeedConfig
        );
    }

    return m_gaussianResidualModel.optimizeObservationSet(preparedObservationSet);
}
