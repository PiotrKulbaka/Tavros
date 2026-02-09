#pragma once

#include <tavros/core/types.hpp>
#include <array>
#include <algorithm>


class fps_meter
{
public:
    static constexpr std::size_t buffer_size = 256;

public:
    fps_meter() noexcept
        : m_buffer{}
        , m_index(0)
        , m_count(0)
    {
    }

    void tick(float delta_time_sec) noexcept
    {
        m_buffer[m_index] = delta_time_sec;
        m_index = (m_index + 1) % buffer_size;

        if (m_count < buffer_size) {
            ++m_count;
        }
    }

    float average_fps() const noexcept
    {
        if (m_count == 0) {
            return 0.0f;
        }

        float sum_dt = 0.0f;

        for (std::size_t i = 0; i < m_count; ++i) {
            sum_dt += m_buffer[i];
        }

        const float avg_dt = sum_dt / static_cast<float>(m_count);
        return 1.0f / avg_dt;
    }

    float median_fps() const noexcept
    {
        if (m_count == 0) {
            return 0.0f;
        }

        std::array<float, buffer_size> temp;
        for (std::size_t i = 0; i < m_count; ++i) {
            temp[i] = m_buffer[i];
        }

        std::sort(temp.begin(), temp.begin() + m_count);

        const std::size_t mid = m_count / 2;

        float median_dt;
        if ((m_count % 2) == 0) {
            median_dt = (temp[mid - 1] + temp[mid]) * 0.5f;
        } else {
            median_dt = temp[mid];
        }

        return 1.0f / median_dt;
    }

private:
    std::array<float, buffer_size> m_buffer;
    std::size_t                    m_index;
    std::size_t                    m_count;
};
