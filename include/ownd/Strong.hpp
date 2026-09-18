#pragma once

#include "detail/ControlBlock.hpp"
#include "detail/PointerControlBlock.hpp"

#include <cstddef>
#include <utility>

namespace ownd {

    template<typename T>
    class Strong;

    template<typename T, typename... Args>
    [[nodiscard]]
    Strong<T> MakeStrong(Args&&... args);

    template<typename T>
    class Strong {
    public:
        constexpr Strong() noexcept = default;
        constexpr Strong(std::nullptr_t) noexcept {};

        Strong(const Strong& o) noexcept
            : m_Pointer(o.m_Pointer)
            , m_ControlBlock(o.m_ControlBlock)
            {
            if (m_ControlBlock != nullptr)
                m_ControlBlock->AddStrong();
        }

        Strong& operator=(const Strong& o) noexcept {
            if (this == &o) return *this;

            Strong copy(o);
            Swap(copy);

            return *this;
        } 

        Strong(Strong&& o) noexcept
            : m_Pointer(std::exchange(o.m_Pointer, nullptr))
            , m_ControlBlock(std::exchange(o.m_ControlBlock, nullptr))
            {}
        
        Strong& operator=(const Strong&& o) noexcept {
            if (this == &o) return *this;

            Reset();

            m_Pointer = std::exchange(o.m_Pointer, nullptr);
            m_ControlBlock = std::exchange(o.m_ControlBlock, nullptr);

            return *this;
        }

        ~Strong() { 
            Reset();
        }

        void Reset() noexcept {
            if (m_ControlBlock != nullptr)
                m_ControlBlock->ReleaseStrong();
        
            m_Pointer = nullptr;
            m_ControlBlock = nullptr;
        }

        void Swap(Strong& other) noexcept {
            std::swap(m_Pointer, other.m_Pointer);
            std::swap(m_ControlBlock, other.m_ControlBlock);
        }

        [[nodiscard]]
        T* Get() const noexcept { return m_Pointer; }

        [[nodiscard]]
        T& operator*() const noexcept { return *m_Pointer; }

        [[nodiscard]]
        T* operator->() const noexcept { return m_Pointer; }

        [[nodiscard]]
        explicit operator bool() const noexcept { return m_Pointer != nullptr; }

        [[nodiscard]]
        std::size_t UseCount() const noexcept {
            if (m_ControlBlock == nullptr) return 0;
            return m_ControlBlock->UseCount();
        }

    private:
        Strong(T* pointer, detail::ControlBlock* controlBlock) noexcept
            : m_Pointer(pointer)
            , m_ControlBlock(controlBlock)
            {}
        
        template<typename U, typename... Args>
        friend Strong<U> MakeStrong(Args&&... args);
    
    private:
        T* m_Pointer{ nullptr };
        detail::ControlBlock* m_ControlBlock{ nullptr };
    };

    template<typename T, typename... Args>
    [[nodiscard]]
    Strong<T> MakeStrong(Args&&... args) {
        auto* controlBlock = new detail::PointerControlBlock<T>(std::forward<Args>(args)...);
        return Strong<T>(controlBlock->Get(), controlBlock);
    }

} // namespace ownd
