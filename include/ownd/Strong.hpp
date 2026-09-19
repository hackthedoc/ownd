#pragma once

#include "detail/ControlBlock.hpp"
#include "detail/InplaceControlBlock.hpp"

#include <cstddef>
#include <type_traits>
#include <utility>
#include <concepts>

namespace ownd {

    template<typename T>
    class Strong;

    template<typename T, typename... Args>
    [[nodiscard]]
    Strong<T> MakeStrong(Args&&... args);

    template<typename T>
    class Strong {
    public:
        static_assert(std::is_object_v<T>, "ownd::Strong<T> requires T to be an object type");
        static_assert(!std::is_array_v<T>, "ownd::Strong<T> does not support arrays yet");

        constexpr Strong() noexcept = default;
        constexpr Strong(std::nullptr_t) noexcept {}
        
        Strong& operator=(std::nullptr_t) noexcept {
            Reset();
            return *this;
        }

        Strong(const Strong& other) noexcept
            : m_Pointer(other.m_Pointer)
            , m_ControlBlock(other.m_ControlBlock)
            {
            if (m_ControlBlock != nullptr)
                m_ControlBlock->AddStrong();
        }

        Strong& operator=(const Strong& other) noexcept {
            if (this == &other) return *this;

            Strong copy(other);
            Swap(copy);

            return *this;
        }
        
        template<typename U>
        requires std::convertible_to<U*, T*>
        Strong(const Strong<U>& other) noexcept 
            : m_Pointer(other.m_Pointer)
            , m_ControlBlock(other.m_ControlBlock)
            {
            if (m_ControlBlock != nullptr)
                m_ControlBlock->AddStrong();
        }
        
        template<typename U>
        requires std::convertible_to<U*, T*>
        Strong& operator=(const Strong<U>& other) noexcept {
            Strong converted(other);
            Swap(converted);
            return *this;
        }

        Strong(Strong&& other) noexcept
            : m_Pointer(std::exchange(other.m_Pointer, nullptr))
            , m_ControlBlock(std::exchange(other.m_ControlBlock, nullptr))
            {}
        
        
        Strong& operator=(Strong&& other) noexcept {
            if (this == &other) return *this;

            Reset();

            m_Pointer = std::exchange(other.m_Pointer, nullptr);
            m_ControlBlock = std::exchange(other.m_ControlBlock, nullptr);

            return *this;
        }
        
        template<typename U>
        requires std::convertible_to<U*, T*>
        Strong(Strong<U>&& other) noexcept
            : m_Pointer(std::exchange(other.m_Pointer, nullptr))
            , m_ControlBlock(std::exchange(other.m_ControlBlock, nullptr))
            {}
        
        template<typename U>
        requires std::convertible_to<U*, T*>
        Strong& operator=(const Strong<U>&& other) noexcept {
            Strong converted(std::move(other));
            Swap(converted);
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

        template<typename U>
        friend class Strong;
    
    private:
        T* m_Pointer{ nullptr };
        detail::ControlBlock* m_ControlBlock{ nullptr };
    };

    template<typename T, typename... Args>
    [[nodiscard]]
    Strong<T> MakeStrong(Args&&... args) {
        auto* controlBlock = new detail::InplaceControlBlock<T>(std::forward<Args>(args)...);
        return Strong<T>(controlBlock->Get(), controlBlock);
    }
    
    template<typename T, typename U>
    requires std::equality_comparable_with<T*, U*>
    [[nodiscard]]
    bool operator==(const Strong<U>& left, const Strong<T>& right) noexcept {
        return left.Get() == right.Get();
    }

    template<typename T>
    [[nodiscard]]
    bool operator==(const Strong<T>& pointer, std::nullptr_t) noexcept {
        return pointer.Get() == nullptr;
    }

    template<typename T>
    [[nodiscard]]
    bool operator==(std::nullptr_t, const Strong<T>& pointer) noexcept {
        return pointer.Get() == nullptr;
    }

    template<typename T>
    void Swap(Strong<T>& left, Strong<T>& right) noexcept {
        left.Swap(right);
    }

} // namespace ownd
