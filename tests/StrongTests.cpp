#include <doctest/doctest.h>
#include <stdexcept>
#include <type_traits>

#include <ownd/Strong.hpp>

namespace {

    struct TestObject {
        TestObject(int initialValue, bool& destroyed)
            : Value(initialValue)
            , Destroyed(destroyed)
            {}
        
        ~TestObject() {
            Destroyed = true;
        }

        int Value;
        bool& Destroyed;
    };

    struct ThrowingObject {
        explicit ThrowingObject(bool& constructorCalled) {
            constructorCalled = true;
            throw std::runtime_error("construction failed");
        }
    };

    struct BaseObject {
        virtual ~BaseObject() = default;

        int Value{ 42 };
    };

    struct DerivedObject final : BaseObject {
        explicit DerivedObject(bool& destroyed) : Destroyed(destroyed) {}

        ~DerivedObject() override { Destroyed = true; }

        bool& Destroyed;
    };

    struct DestructionTracker {
        explicit DestructionTracker(int& destructionCount) : DestructionCount(destructionCount) {}

        ~DestructionTracker() { DestructionCount++; }

        int& DestructionCount;
    };

    struct TrackedValue {
        TrackedValue(int value, int& destructionCount) : Value(value), DestructionCount(destructionCount) {}

        ~TrackedValue() { DestructionCount++; }

        int Value;
        int& DestructionCount;
    };

    struct NonVirtualBase {
        explicit NonVirtualBase(int& destructionCount) : DestructionCount(destructionCount) {}

        ~NonVirtualBase() { ++DestructionCount; }

        int& DestructionCount;
    };

    struct TrackedDerived final : NonVirtualBase {
        TrackedDerived(int& baseDestructionCount, int& derivedDestructionCount) : NonVirtualBase(baseDestructionCount), DerivedDestructionCount(derivedDestructionCount) {}

        ~TrackedDerived() { ++DerivedDestructionCount; }

        int& DerivedDestructionCount;
    };

    struct MoveOnlyPayload {
        explicit MoveOnlyPayload(std::unique_ptr<int> value) : Value(std::move(value)) {}

        std::unique_ptr<int> Value;
    };

    struct ImmovablePayload {
        explicit ImmovablePayload(int value) : Value(value) {}

        ImmovablePayload(const ImmovablePayload&) = delete;
        ImmovablePayload& operator=(const ImmovablePayload&) = delete;

        ImmovablePayload(ImmovablePayload&&) = delete;
        ImmovablePayload& operator=(ImmovablePayload&&) = delete;

        int Value;
    };

    struct alignas(64) OverAlignedPayload {
        explicit OverAlignedPayload(int value) : Value(value) {}

        int Value;
    };

}

static_assert(std::is_constructible_v<ownd::Strong<BaseObject>, const ownd::Strong<DerivedObject>&>);

static_assert(std::is_constructible_v<ownd::Strong<const BaseObject>, const ownd::Strong<BaseObject>&>);

static_assert(!std::is_constructible_v<ownd::Strong<DerivedObject>, const ownd::Strong<BaseObject>&>);

static_assert(!std::is_constructible_v<ownd::Strong<BaseObject>, const ownd::Strong<const BaseObject>&>);

using StrongInt = ownd::Strong<int>;

static_assert(std::is_nothrow_default_constructible_v<StrongInt>);

static_assert(std::is_nothrow_copy_constructible_v<StrongInt>);

static_assert(std::is_nothrow_move_constructible_v<StrongInt>);

static_assert(std::is_nothrow_copy_assignable_v<StrongInt>);

static_assert(std::is_nothrow_move_assignable_v<StrongInt>);

static_assert(noexcept(std::declval<StrongInt&>().Reset()));

static_assert(noexcept(std::declval<StrongInt&>().Swap(std::declval<StrongInt&>())));

static_assert(noexcept(std::declval<const StrongInt&>().Get()));

static_assert(noexcept(std::declval<const StrongInt&>().UseCount()));

static_assert(noexcept(std::declval<StrongInt&>() == std::declval<StrongInt&>()));

static_assert(std::is_nothrow_constructible_v<ownd::Strong<BaseObject>, const ownd::Strong<DerivedObject>&>);

static_assert(std::is_nothrow_constructible_v<ownd::Strong<BaseObject>, ownd::Strong<DerivedObject>&&>);

TEST_CASE("MakeStrong creates and owns an object") {
    bool destroyed = false;

    {
        auto object = ownd::MakeStrong<TestObject>(42, destroyed);

        CHECK(object);
        CHECK(object.Get() != nullptr);
        CHECK(object->Value == 42);
        CHECK((*object).Value == 42);
        CHECK(object.UseCount() == 1);
        CHECK_FALSE(destroyed);
    }

    CHECK(destroyed);
}

TEST_CASE("Strong can transfer ownership") {
    bool destroyed = false;

    auto original = ownd::MakeStrong<TestObject>(42, destroyed);
    auto copy = std::move(original);

    CHECK_FALSE(original);
    CHECK(original.UseCount() == 0);

    CHECK(copy);
    CHECK(copy.UseCount() == 1);
    CHECK(copy->Value == 42);
    CHECK_FALSE(destroyed);
}

TEST_CASE("Strong shares ownership when copied") {
    bool destroyed = false;

    auto first = ownd::MakeStrong<TestObject>(42, destroyed);

    {
        auto second = first;

        CHECK(first.Get() == second.Get());
        CHECK(first.UseCount() == 2);
        CHECK(second.UseCount() == 2);
        CHECK_FALSE(destroyed);

        first.Reset();

        CHECK_FALSE(first);
        CHECK(first.UseCount() == 0);
        CHECK(second.UseCount() == 1);
        CHECK_FALSE(destroyed);
    }

    CHECK(destroyed);
}

TEST_CASE("Strong copy assignment releases previous ownership") {
    bool firstDestroyed = false;
    bool secondDestroyed = false;

    auto first = ownd::MakeStrong<TestObject>(1, firstDestroyed);
    auto second = ownd::MakeStrong<TestObject>(2, secondDestroyed);

    first = second;

    CHECK(firstDestroyed);
    CHECK_FALSE(secondDestroyed);

    CHECK(first.Get() == second.Get());
    CHECK(first.UseCount() == 2);
    CHECK(second.UseCount() == 2);
    CHECK(first->Value == 2);
}

TEST_CASE("Strong compares with nullptr") {
    ownd::Strong<TestObject> empty;

    CHECK(empty == nullptr);
    CHECK(nullptr == empty);
    CHECK_FALSE(empty != nullptr);

    bool destroyed = false;
    auto object = ownd::MakeStrong<TestObject>(42, destroyed);

    CHECK(object != nullptr);
    CHECK(nullptr != object);
}

TEST_CASE("Strong compares stored pointers") {
    bool firstDestroyed = false;
    bool secondDestroyed = false;

    auto first = ownd::MakeStrong<TestObject>(42, firstDestroyed);
    auto copy = first;
    auto separate = ownd::MakeStrong<TestObject>(42, secondDestroyed);

    CHECK(first == copy);
    CHECK_FALSE(first != copy);

    CHECK(first != separate);
    CHECK_FALSE(first == separate);
}

TEST_CASE("Strong swaps ownership") {
    bool firstDestroyed = false;
    bool secondDestroyed = false;

    auto first = ownd::MakeStrong<TestObject>(1, firstDestroyed);
    auto second = ownd::MakeStrong<TestObject>(2, secondDestroyed);

    TestObject* firstPointer = first.Get();
    TestObject* secondPointer = second.Get();

    ownd::Swap(first, second);

    CHECK(first.Get() == secondPointer);
    CHECK(second.Get() == firstPointer);

    CHECK(first->Value == 2);
    CHECK(second->Value == 1);

    CHECK(first.UseCount() == 1);
    CHECK(second.UseCount() == 1);
}

TEST_CASE("MakeStrong propagates constructors exceptions") {
    bool constructorCalled = false;

    CHECK_THROWS_AS(static_cast<void>(ownd::MakeStrong<ThrowingObject>(constructorCalled)), std::runtime_error);
    CHECK(constructorCalled);
}

TEST_CASE("Strong supports derived to base copy conversion") {
    bool destroyed = false;

    auto derived = ownd::MakeStrong<DerivedObject>(destroyed);
    ownd::Strong<BaseObject> base = derived;

    CHECK(derived.UseCount() == 2);
    CHECK(base.UseCount() == 2);
    CHECK(base->Value == 42);

    derived.Reset();

    CHECK(base.UseCount() == 1);
    CHECK_FALSE(destroyed);

    base.Reset();

    CHECK(destroyed);
}

TEST_CASE("Strong supports derived to base move conversion") {
    bool destroyed = false;

    auto derived = ownd::MakeStrong<DerivedObject>(destroyed);
    DerivedObject* originalPointer = derived.Get();

    ownd::Strong<BaseObject> base = std::move(derived);

    CHECK_FALSE(derived);
    CHECK(derived.UseCount() == 0);

    CHECK(base.Get() == originalPointer);
    CHECK(base.UseCount() == 1);
    CHECK_FALSE(destroyed);
}

TEST_CASE("Strong supports conversion to const") {
    bool destroyed = false;

    auto mutableObject = ownd::MakeStrong<DerivedObject>(destroyed);
    ownd::Strong<const DerivedObject> readOnly = mutableObject;

    CHECK(mutableObject.UseCount() == 2);
    CHECK(readOnly.UseCount() == 2);
    CHECK(readOnly->Value == 42);
}

TEST_CASE("Strong compares compatible pointer types") {
    bool destroyed = false;

    auto derived = ownd::MakeStrong<DerivedObject>(destroyed);
    ownd::Strong<BaseObject> base = derived;
    ownd::Strong<const DerivedObject> readOnly = derived;

    const std::size_t useCount = derived.UseCount();

    CHECK(derived == base);
    CHECK(base == derived);

    CHECK(derived == readOnly);
    CHECK(readOnly == derived);

    CHECK_FALSE(derived != base);
    CHECK_FALSE(derived != readOnly);

    CHECK(derived.UseCount() == useCount);
    CHECK(base.UseCount() == useCount);
    CHECK(readOnly.UseCount() == useCount);
}

TEST_CASE("Strong compares different allocations as unequal") {
    bool firstDestroyed = false;
    bool secondDestroyed = false;

    auto first = ownd::MakeStrong<DerivedObject>(firstDestroyed);

    auto second = ownd::MakeStrong<DerivedObject>(secondDestroyed);

    CHECK(first != second);
    CHECK_FALSE(first == second);
}

TEST_CASE("Strong supports assignment from nullptr") {
    bool destroyed = false;

    auto object = ownd::MakeStrong<DerivedObject>(destroyed);

    CHECK(object != nullptr);
    CHECK(object.UseCount() == 1);

    object = nullptr;

    CHECK(object == nullptr);
    CHECK(object.UseCount() == 0);
    CHECK(destroyed);

    object = nullptr;

    CHECK(object == nullptr);
    CHECK(object.UseCount() == 0);
}

TEST_CASE("Empty Strong handles support modifiers") {
    ownd::Strong<int> first;
    ownd::Strong<int> second;

    CHECK(first == second);
    CHECK(first == nullptr);
    CHECK(second == nullptr);

    first.Reset();
    first.Reset();

    ownd::Swap(first, second);

    CHECK(first == nullptr);
    CHECK(second == nullptr);
    CHECK(first.UseCount() == 0);
    CHECK(second.UseCount() == 0);
}

TEST_CASE("Strong destroys its payload exactly once") {
    int destructionCount = 0;

    auto first = ownd::MakeStrong<DestructionTracker>(destructionCount);

    CHECK(first.UseCount() == 1);

    auto second = first;
    auto third = second;

    CHECK(first.UseCount() == 3);
    CHECK(second.UseCount() == 3);
    CHECK(third.UseCount() == 3);

    auto moved = std::move(third);

    CHECK_FALSE(third);
    CHECK(third.UseCount() == 0);
    CHECK(moved.UseCount() == 3);
    CHECK(destructionCount == 0);

    second.Reset();

    CHECK(first.UseCount() == 2);
    CHECK(moved.UseCount() == 2);
    CHECK(destructionCount == 0);

    first.Reset();

    CHECK(moved.UseCount() == 1);
    CHECK(destructionCount == 0);

    moved.Reset();

    CHECK(destructionCount == 1);

    moved.Reset();

    CHECK(destructionCount == 1);
}

TEST_CASE("Strong move assignment releases previous ownership") {
    int firstDestructionCount = 0;
    int secondDestructionCount = 0;

    auto first = ownd::MakeStrong<TrackedValue>(1, firstDestructionCount);

    auto second = ownd::MakeStrong<TrackedValue>(2, secondDestructionCount);

    TrackedValue* secondPointer = second.Get();

    first = std::move(second);

    CHECK(firstDestructionCount == 1);
    CHECK(secondDestructionCount == 0);

    CHECK_FALSE(second);
    CHECK(second.UseCount() == 0);

    CHECK(first.Get() == secondPointer);
    CHECK(first->Value == 2);
    CHECK(first.UseCount() == 1);

    first.Reset();

    CHECK(firstDestructionCount == 1);
    CHECK(secondDestructionCount == 1);
}

TEST_CASE("Strong survives self move assignment") {
    int destructionCount = 0;

    auto object = ownd::MakeStrong<DestructionTracker>(destructionCount);

    DestructionTracker* originalPointer = object.Get();

    ownd::Strong<DestructionTracker>* alias = &object;
    object = std::move(*alias);

    CHECK(object);
    CHECK(object.Get() == originalPointer);
    CHECK(object.UseCount() == 1);
    CHECK(destructionCount == 0);

    object.Reset();

    CHECK(destructionCount == 1);
}

TEST_CASE("Strong supports copying and moving empty handles") {
    ownd::Strong<DestructionTracker> empty;

    auto copied = empty;

    CHECK_FALSE(empty);
    CHECK_FALSE(copied);
    CHECK(empty.UseCount() == 0);
    CHECK(copied.UseCount() == 0);

    auto moved = std::move(empty);

    CHECK_FALSE(empty);
    CHECK_FALSE(moved);
    CHECK(empty.UseCount() == 0);
    CHECK(moved.UseCount() == 0);

    ownd::Strong<DestructionTracker> copyAssigned;
    copyAssigned = copied;

    CHECK_FALSE(copyAssigned);
    CHECK(copyAssigned.UseCount() == 0);

    ownd::Strong<DestructionTracker> moveAssigned;
    moveAssigned = std::move(moved);

    CHECK_FALSE(moved);
    CHECK_FALSE(moveAssigned);
    CHECK(moveAssigned.UseCount() == 0);
}

TEST_CASE("Assigning an empty Strong releases previous ownership") {
    int destructionCount = 0;

    auto object = ownd::MakeStrong<DestructionTracker>(destructionCount);

    ownd::Strong<DestructionTracker> empty;

    object = empty;

    CHECK_FALSE(object);
    CHECK(object.UseCount() == 0);
    CHECK(destructionCount == 1);
}

TEST_CASE("Strong destroys the concrete type through a non-virtual base") {
    int baseDestructionCount = 0;
    int derivedDestructionCount = 0;

    auto derived = ownd::MakeStrong<TrackedDerived>(baseDestructionCount, derivedDestructionCount);

    ownd::Strong<NonVirtualBase> base = derived;

    CHECK(derived.UseCount() == 2);
    CHECK(base.UseCount() == 2);

    derived.Reset();

    CHECK(base.UseCount() == 1);
    CHECK(baseDestructionCount == 0);
    CHECK(derivedDestructionCount == 0);

    base.Reset();

    CHECK(baseDestructionCount == 1);
    CHECK(derivedDestructionCount == 1);
}

TEST_CASE("MakeStrong perfectly forwards move-only arguments") {
    auto argument = std::make_unique<int>(42);
    int* originalPointer = argument.get();

    auto object = ownd::MakeStrong<MoveOnlyPayload>(std::move(argument));

    CHECK(argument == nullptr);
    REQUIRE(object->Value != nullptr);

    CHECK(object->Value.get() == originalPointer);
    CHECK(*object->Value == 42);
}

TEST_CASE("MakeStrong constructs an immovable payload in place") {
    auto object =  ownd::MakeStrong<ImmovablePayload>(42);

    CHECK(object);
    CHECK(object.UseCount() == 1);
    CHECK(object->Value == 42);
}

TEST_CASE("MakeStrong respects over-aligned payload alignment") {
    auto object = ownd::MakeStrong<OverAlignedPayload>(42);

    const auto address = reinterpret_cast<std::uintptr_t>(object.Get());

    CHECK(address % alignof(OverAlignedPayload) == 0);
    CHECK(object->Value == 42);
}
