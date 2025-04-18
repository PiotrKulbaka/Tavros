#include <tavros/core/timer.hpp>

using namespace tavros::core;

timer::timer() noexcept
{
    start();
}

void timer::start() noexcept
{
    m_start = clock::now();
    m_accumulated = duration(0);
    m_is_paused = false;
}

void timer::pause() noexcept
{
    if (!m_is_paused) {
        m_accumulated += clock::now() - m_start;
        m_is_paused = true;
    }
}

void timer::unpause() noexcept
{
    if (m_is_paused) {
        m_start = clock::now();
        m_is_paused = false;
    }
}

bool timer::is_paused() const noexcept
{
    return m_is_paused;
}

timer::duration timer::elapsed_duration() const noexcept
{
    return m_is_paused ? m_accumulated : m_accumulated + (clock::now() - m_start);
}
