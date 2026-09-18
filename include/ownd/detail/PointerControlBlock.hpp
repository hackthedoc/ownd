#pragma once

#include "ControlBlock.hpp"

#include <utility>

namespace ownd::detail {

    template<typename T>
    class PointerControlBlock : public ControlBlock {
    public:
        template<typename... Args>
        explicit PointerControlBlock(Args&&... args)
            : m_Pointer(new T(std::forward<Args>(args)...))
            {}
        
        [[nodiscard]]
        T* Get() const noexcept {
            return m_Pointer;
        }

    private:
        void DestroyPayload() noexcept override {
            delete m_Pointer;
            m_Pointer = nullptr;
        }
        
        void DestroyBlock() noexcept override{
            delete this;
        }

    private:
        T* m_Pointer;
    };

} // namespace ownd::detail
