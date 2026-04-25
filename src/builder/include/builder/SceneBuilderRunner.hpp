#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

#include "builder/types/runner.hpp"
#include "config/types/builder.hpp"

namespace edgevision::model::frame {
    class FrameStore;
} // namespace edgevision::model::frame

namespace edgevision::model::scene {
    class SceneVersionStore;
    class SharedScene;
} // namespace edgevision::model::scene

namespace edgevision::builder {

    struct FrameBuildResult;
    class SceneBuilder;

    class SceneBuilderRunner final {
      public:
        SceneBuilderRunner(
            model::frame::FrameStore& frameStore,
            model::scene::SharedScene& sharedScene,
            model::scene::SceneVersionStore& sceneVersionStore,
            const config::BuilderRuntimeConfig& config = {}
        );
        ~SceneBuilderRunner();

        SceneBuilderRunner(const SceneBuilderRunner&) = delete;
        SceneBuilderRunner& operator=(const SceneBuilderRunner&) = delete;
        SceneBuilderRunner(SceneBuilderRunner&& other) noexcept = delete;
        SceneBuilderRunner& operator=(SceneBuilderRunner&& other) noexcept = delete;

        /// Starts the background build loop.
        [[nodiscard]] bool start();
        /// Requests that the background build loop stop.
        void requestStop();
        /// Waits for the background build loop to exit.
        void join();
        /// Returns whether the background build loop is currently running.
        [[nodiscard]] bool running() const;
        /// Returns the latest builder runner status snapshot.
        [[nodiscard]] SceneBuilderRunnerStatus status() const;

      private:
        void runLoop();
        void recordResult(const FrameBuildResult& result);
        void setRunning(bool running);

        model::frame::FrameStore& m_frameStore;
        std::unique_ptr<SceneBuilder> m_builder{};
        config::BuilderRuntimeConfig m_config{};
        std::atomic_bool m_stopRequested{false};
        std::atomic_bool m_running{false};
        mutable std::mutex m_statusMutex{};
        std::thread m_thread{};
        SceneBuilderRunnerStatus m_status{};
    };

} // namespace edgevision::builder
