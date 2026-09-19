#pragma once

#include "ControlBlock.hpp"

#include <cstddef>
#include <memory>
#include <new>
#include <utility>

namespace ownd::detail {

    template<typename T>
    class InplaceControlBlock final : public ControlBlock {
    public:
        template<typename... Args>
        explicit InplaceControlBlock(Args&&... args) {
            std::construct_at(StorageAddress(), std::forward<Args>(args)...);
        }

        [[nodiscard]]
        T* Get() noexcept { return std::launder(StorageAddress()); }

        [[nodiscard]]
        const T* Get() const noexcept { return std::launder(StorageAddress()); }

    private:
        [[nodiscard]]
        T* StorageAddress() noexcept { return reinterpret_cast<T*>(m_Storage); }

        [[nodiscard]]
        const T* StorageAddress() const noexcept { return reinterpret_cast<const T*>(m_Storage); }
        
        void DestroyPayload() noexcept override { std::destroy_at(Get()); }
        
        void DestroyBlock() noexcept override { delete this; }
    
    private:
        alignas(T) std::byte m_Storage[sizeof(T)];
    };

} // namespace ownd::detail
