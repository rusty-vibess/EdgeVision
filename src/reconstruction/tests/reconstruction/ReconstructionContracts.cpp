#include <cstddef>
#include <iostream>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "interfaces/GaussianResidualModel.hpp"
#include "interfaces/TsdfRaycaster.hpp"
#include "reconstruction/ReconstructionPipeline.hpp"

namespace {
    int gFailures = 0;

    void recordFailure(
        std::string_view message,
        const std::source_location& location = std::source_location::current()
    ) {
        std::cerr << location.file_name() << ':' << location.line() << ": " << message << '\n';
        ++gFailures;
    }

    void expectTrue(
        bool value,
        std::string_view message,
        const std::source_location& location = std::source_location::current()
    ) {
        if (!value) {
            recordFailure(message, location);
        }
    }

    template <typename T, typename U>
    void expectEq(
        const T& lhs,
        const U& rhs,
        std::string_view message,
        const std::source_location& location = std::source_location::current()
    ) {
        if (!(lhs == rhs)) {
            recordFailure(message, location);
        }
    }

    struct FakeTsdfRaycaster final : public ITsdfRaycaster {
        mutable int raycastCalls = 0;
        TsdfRaycastBuffers result{};

        [[nodiscard]] TsdfRaycastBuffers raycast(const RgbdFrameView&, bool) const override {
            ++raycastCalls;
            return result;
        }
    };

    struct FakeGaussianResidualModel final : public IGaussianResidualModel {
        int seedCalls = 0;
        int optimizeCalls = 0;
        std::string lastSeedObservationId{};
        float optimizeResult = 0.0f;
        HybridRenderOutput renderResult{};
        OptimizationObservationSet lastObservationSet{};

        void seedFromResidual(
            const ReconstructionObservation& observation,
            const ResidualSeedConfig&
        ) override {
            ++seedCalls;
            lastSeedObservationId = observation.observationId;
        }

        [[nodiscard]] HybridRenderOutput renderHybrid(const HybridRenderInput& input) override {
            renderResult.size = input.observation.frame.rgb.size;
            return renderResult;
        }

        [[nodiscard]] float optimizeObservationSet(
            const OptimizationObservationSet& observationSet
        ) override {
            ++optimizeCalls;
            lastObservationSet = observationSet;
            return optimizeResult;
        }
    };

    ReconstructionObservation makeObservation(
        std::string observationId,
        int sequenceIndex,
        bool hasRaycast
    ) {
        static const float rgbValues[] = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f};
        static const float depthValues[] = {1.0f, 2.0f};
        static const float vertexValues[] = {0.0f, 0.1f, 0.2f, 1.0f, 1.1f, 1.2f};
        static const float confidenceValues[] = {0.8f, 0.9f};

        ImageSize size{640, 480};
        CameraIntrinsics intrinsics{525.0f, 525.0f, 319.5f, 239.5f};
        Pose4f pose{};
        pose.matrix[0] = 1.0f;
        pose.matrix[5] = 1.0f;
        pose.matrix[10] = 1.0f;
        pose.matrix[15] = 1.0f;

        ReconstructionObservation observation{};
        observation.observationId = std::move(observationId);
        observation.sequenceIndex = sequenceIndex;
        observation.hasRaycast = hasRaycast;
        observation.frame.rgb = RgbFrameView{size, BufferView<float>(rgbValues, 6)};
        observation.frame.depth = DepthFrameView{size, BufferView<float>(depthValues, 2)};
        observation.frame.intrinsics = intrinsics;
        observation.frame.cameraToWorld = pose;
        observation.frame.cameraToWorldSlam = pose;
        observation.frame.hasDepth = true;
        observation.raycast.size = size;
        observation.raycast.colorMap = BufferView<float>(rgbValues, 6);
        observation.raycast.vertexMap = BufferView<float>(vertexValues, 6);
        observation.raycast.confidenceMap = BufferView<float>(confidenceValues, 2);
        observation.raycast.depthMap = BufferView<float>(depthValues, 2);
        observation.raycast.normalMap = BufferView<float>(vertexValues, 6);
        return observation;
    }

    void testBufferView() {
        const int values[] = {4, 5, 6};
        BufferView<int> view(values, 3);

        expectTrue(!view.empty(), "BufferView should report non-empty");
        expectEq(view.size(), std::size_t{3}, "BufferView size should match the input span");
        expectEq(view.data(), values, "BufferView should preserve the input pointer");
        expectEq(view[1], 5, "BufferView index access should preserve values");
        expectEq(*view.begin(), 4, "BufferView begin iterator should point at the first element");
        expectEq(*(view.end() - 1), 6, "BufferView end iterator should reach the last element");
    }

    void testObservationTypes() {
        ReconstructionObservation currentObservation = makeObservation("current", 7, true);
        ReconstructionObservation localObservation = makeObservation("local", 3, false);

        OptimizationObservationSet observationSet{};
        observationSet.currentObservation = currentObservation;
        observationSet.hasCurrentObservation = true;
        observationSet.includeCurrentObservationInOptimization = true;
        observationSet.localObservations.push_back(localObservation);

        ReconstructionPipelineConfig config{};
        config.localOptimizationInterval = 4;
        config.includeCurrentObservationInOptimization = false;

        HybridRenderInput renderInput{};
        renderInput.observation = currentObservation;
        renderInput.deltaDepth = 0.1f;

        expectEq(
            observationSet.currentObservation.observationId,
            std::string("current"),
            "Current observation id should be preserved"
        );
        expectEq(
            observationSet.localObservations.size(),
            std::size_t{1},
            "The local observation set should track inserted observations"
        );
        expectTrue(
            !config.includeCurrentObservationInOptimization,
            "Pipeline config should preserve the optimization flag"
        );
        expectEq(
            renderInput.observation.sequenceIndex,
            7,
            "Hybrid render input should preserve the current observation"
        );
    }

    void testPrepareObservation() {
        FakeTsdfRaycaster raycaster;
        raycaster.result = makeObservation("raycast", 1, true).raycast;
        FakeGaussianResidualModel gaussianModel;
        ReconstructionPipeline pipeline(raycaster, gaussianModel);

        const ReconstructionObservation noRaycastObservation = makeObservation("seed", 2, false);
        const ReconstructionObservation preparedObservation =
            pipeline.prepareObservation(noRaycastObservation);

        expectTrue(preparedObservation.hasRaycast, "prepareObservation should populate raycasts");
        expectEq(raycaster.raycastCalls, 1, "prepareObservation should invoke the TSDF raycaster");

        const ReconstructionObservation existingRaycastObservation =
            makeObservation("cached", 3, true);
        const ReconstructionObservation cachedObservation =
            pipeline.prepareObservation(existingRaycastObservation);

        expectTrue(
            cachedObservation.hasRaycast, "prepareObservation should preserve existing raycasts"
        );
        expectEq(
            raycaster.raycastCalls,
            1,
            "prepareObservation should not reraycast observations that already have one"
        );
    }

    void testPrepareObservationSet() {
        FakeTsdfRaycaster raycaster;
        raycaster.result = makeObservation("raycast", 4, true).raycast;
        FakeGaussianResidualModel gaussianModel;
        ReconstructionPipeline pipeline(raycaster, gaussianModel);

        OptimizationObservationSet observationSet{};
        observationSet.currentObservation = makeObservation("current", 4, false);
        observationSet.hasCurrentObservation = true;
        observationSet.localObservations.push_back(makeObservation("local-a", 5, false));
        observationSet.localObservations.push_back(makeObservation("local-b", 6, true));
        observationSet.keyframeObservations.push_back(makeObservation("key-a", 7, false));

        const OptimizationObservationSet preparedObservationSet =
            pipeline.prepareObservationSet(observationSet);

        expectTrue(
            preparedObservationSet.currentObservation.hasRaycast,
            "prepareObservationSet should prepare the current observation"
        );
        expectTrue(
            preparedObservationSet.localObservations[0].hasRaycast,
            "prepareObservationSet should prepare local observations without cached raycasts"
        );
        expectTrue(
            preparedObservationSet.localObservations[1].hasRaycast,
            "prepareObservationSet should preserve cached local raycasts"
        );
        expectTrue(
            preparedObservationSet.keyframeObservations[0].hasRaycast,
            "prepareObservationSet should prepare keyframe observations"
        );
        expectEq(
            raycaster.raycastCalls,
            3,
            "prepareObservationSet should raycast only the observations missing cached data"
        );
    }

    void testRunOptimizationCycle() {
        FakeTsdfRaycaster raycaster;
        raycaster.result = makeObservation("raycast", 8, true).raycast;
        FakeGaussianResidualModel gaussianModel;
        gaussianModel.optimizeResult = 1.25f;

        ReconstructionPipelineConfig config{};
        config.optimizeWithKeyframes = false;
        config.includeCurrentObservationInOptimization = false;
        config.residualSeedConfig.colorErrorThreshold = 0.05f;

        ReconstructionPipeline pipeline(raycaster, gaussianModel, config);

        OptimizationObservationSet observationSet{};
        observationSet.currentObservation = makeObservation("current", 8, false);
        observationSet.hasCurrentObservation = true;
        observationSet.localObservations.push_back(makeObservation("local", 9, false));
        observationSet.keyframeObservations.push_back(makeObservation("key", 10, false));

        const float optimizeResult = pipeline.runOptimizationCycle(observationSet);

        expectEq(optimizeResult, 1.25f, "runOptimizationCycle should return the model loss");
        expectEq(gaussianModel.seedCalls, 1, "runOptimizationCycle should seed once per cycle");
        expectEq(
            gaussianModel.lastSeedObservationId,
            std::string("current"),
            "runOptimizationCycle should seed from the current observation"
        );
        expectEq(
            gaussianModel.optimizeCalls,
            1,
            "runOptimizationCycle should invoke optimization exactly once"
        );
        expectTrue(
            !gaussianModel.lastObservationSet.includeCurrentObservationInOptimization,
            "The prepared observation set should inherit the pipeline optimization policy"
        );
        expectTrue(
            gaussianModel.lastObservationSet.currentObservation.hasRaycast,
            "The optimization set should include the prepared current observation"
        );
        expectTrue(
            gaussianModel.lastObservationSet.localObservations[0].hasRaycast,
            "The optimization set should include prepared local observations"
        );
        expectTrue(
            gaussianModel.lastObservationSet.keyframeObservations.empty(),
            "The pipeline should drop keyframes when keyframe optimization is disabled"
        );
    }
} // namespace

int main() {
    testBufferView();
    testObservationTypes();
    testPrepareObservation();
    testPrepareObservationSet();
    testRunOptimizationCycle();

    if (gFailures != 0) {
        std::cerr << "Tests failed: " << gFailures << '\n';
        return 1;
    }

    std::cout << "All tests passed\n";
    return 0;
}
