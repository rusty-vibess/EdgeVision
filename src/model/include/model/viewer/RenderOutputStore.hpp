#pragma once

#include <cstddef>
#include <deque>
#include <optional>
#include <shared_mutex>
#include <vector>

#include "model/viewer/types/output.hpp"

namespace edgevision::model::viewer {

    /// Stores the latest render output and a bounded history of recent outputs.
    class RenderOutputStore final {
      public:
        explicit RenderOutputStore(std::size_t capacity = 8);
        ~RenderOutputStore();

        RenderOutputStore(const RenderOutputStore&) = delete;
        RenderOutputStore& operator=(const RenderOutputStore&) = delete;

        /// Publishes `output` as the latest result and appends it to recent history.
        void publish(RenderOutput output);

        /// Returns the latest published render output, if any.
        [[nodiscard]] std::optional<RenderOutput> latest() const;
        /// Returns up to `count` of the most recent published outputs in oldest-first order.
        [[nodiscard]] std::vector<RenderOutput> recent(std::size_t count) const;

      private:
        std::size_t m_capacity = 8;
        mutable std::shared_mutex m_mutex{};
        std::deque<RenderOutput> m_history{};
        std::optional<RenderOutput> m_latestOutput{};
    };

} // namespace edgevision::model::viewer
