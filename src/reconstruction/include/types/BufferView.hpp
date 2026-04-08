#pragma once

#include <cstddef>
#include <torch/torch.h>

template <typename T>
class BufferView {
  public:
    constexpr BufferView() = default;

    constexpr BufferView(const T* data, std::size_t elementCount)
        : m_data(data), m_elementCount(elementCount) {}

    [[nodiscard]] constexpr const T* data() const {
        return m_data;
    }

    [[nodiscard]] constexpr std::size_t size() const {
        return m_elementCount;
    }

    [[nodiscard]] constexpr bool empty() const {
        return m_elementCount == 0;
    }

    [[nodiscard]] constexpr const T& operator[](std::size_t index) const {
        return m_data[index];
    }

    [[nodiscard]] constexpr const T* begin() const {
        return m_data;
    }

    [[nodiscard]] constexpr const T* end() const {
        if (m_data == nullptr) {
            return nullptr;
        }

        return m_data + m_elementCount;
    }

  private:
    const T* m_data = nullptr;
    std::size_t m_elementCount = 0;
};
