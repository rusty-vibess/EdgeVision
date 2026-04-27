#include "model/viewer/RenderOutputStore.hpp"

#include <algorithm>
#include <mutex>

namespace edgevision::model::viewer {

    RenderOutputStore::RenderOutputStore(std::size_t capacity)
        : m_capacity(std::max<std::size_t>(1, capacity)) {}

    RenderOutputStore::~RenderOutputStore() = default;

    void RenderOutputStore::publish(RenderOutput output) {
        std::unique_lock lock(m_mutex);

        m_history.push_back(output);
        m_latestOutput = std::move(output);

        while (m_history.size() > m_capacity) {
            m_history.pop_front();
        }
    }

    std::optional<RenderOutput> RenderOutputStore::latest() const {
        std::shared_lock lock(m_mutex);
        return m_latestOutput;
    }

    std::vector<RenderOutput> RenderOutputStore::recent(std::size_t count) const {
        std::shared_lock lock(m_mutex);

        const std::size_t clampedCount = std::min(count, m_history.size());
        const std::size_t start = m_history.size() - clampedCount;

        std::vector<RenderOutput> outputs{};
        outputs.reserve(clampedCount);
        for (std::size_t index = start; index < m_history.size(); ++index) {
            outputs.push_back(m_history[index]);
        }

        return outputs;
    }

} // namespace edgevision::model::viewer
