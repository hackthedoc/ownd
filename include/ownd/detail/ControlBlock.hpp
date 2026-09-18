#pragma once

#include <atomic>
#include <cstddef>

namespace ownd::detail {

class ControlBlock {
public:
    ControlBlock() = default;

    ControlBlock(const ControlBlock&) = delete;
    ControlBlock& operator=(const ControlBlock&) = delete;

    ControlBlock(ControlBlock&&) = delete;
    ControlBlock& operator=(ControlBlock&&) = delete;

    void AddStrong() noexcept {
        m_StrongCount.fetch_add(1, std::memory_order_relaxed);
    }

    void ReleaseStrong() noexcept {
        if (m_StrongCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            DestroyPayload();
            DestroyBlock();
        }
    }

    [[nodiscard]]
    std::size_t UseCount() const noexcept {
        return m_StrongCount.load(std::memory_order_relaxed);
    }

protected:
    virtual ~ControlBlock() = default;

private:
    virtual void DestroyPayload() noexcept = 0;
    virtual void DestroyBlock() noexcept = 0;

private:
    std::atomic<size_t> m_StrongCount{1};
};

} // namespace ownd::detail
