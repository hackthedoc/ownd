#include <ownd/Strong.hpp>

#include <doctest/doctest.h>

#include <cstdint>
#include <memory>
#include <utility>

namespace {

    struct DestructionTracker {
        explicit DestructionTracker(int& destructionCount) : DestructionCount(destructionCount) {}

        ~DestructionTracker() { DestructionCount++; }

        int& DestructionCount;
    };

    struct TrackedValue {
        TrackedValue(int value, int& destructionCount) : Value(value) , DestructionCount(destructionCount) {}

        ~TrackedValue() { DestructionCount++; }

        int Value;
        int& DestructionCount;
    };

    struct NonVirtualBase {
        explicit NonVirtualBase(int& destructionCount) : DestructionCount(destructionCount) {}

        ~NonVirtualBase() { DestructionCount++; }

        int& DestructionCount;
    };

    struct TrackedDerived final : NonVirtualBase {
        TrackedDerived(int& baseDestructionCount, int& derivedDestructionCount) : NonVirtualBase(baseDestructionCount), DerivedDestructionCount(derivedDestructionCount) {}

        ~TrackedDerived() { DerivedDestructionCount++; }

        int& DerivedDestructionCount;
    };

    struct MoveOnlyPayload {
        explicit MoveOnlyPayload(std::unique_ptr<int> value)
            : Value(std::move(value))
            {}

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

TEST_CASE("Strong destroys its payload exactly once") {
    int destructionCount = 0;

    auto first = ownd::MakeStrong<DestructionTracker>(destructionCount);

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

    auto* alias = &object;
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
    auto moved = std::move(empty);

    CHECK_FALSE(empty);
    CHECK_FALSE(copied);
    CHECK_FALSE(moved);

    CHECK(empty.UseCount() == 0);
    CHECK(copied.UseCount() == 0);
    CHECK(moved.UseCount() == 0);

    ownd::Strong<DestructionTracker> copyAssigned;
    copyAssigned = copied;

    ownd::Strong<DestructionTracker> moveAssigned;
    moveAssigned = std::move(moved);

    CHECK_FALSE(copyAssigned);
    CHECK_FALSE(moveAssigned);
    CHECK(copyAssigned.UseCount() == 0);
    CHECK(moveAssigned.UseCount() == 0);
}

TEST_CASE("Assigning an empty Strong releases previous ownership") {
    int copyDestructionCount = 0;
    int moveDestructionCount = 0;

    auto copyTarget = ownd::MakeStrong<DestructionTracker>(copyDestructionCount);

    ownd::Strong<DestructionTracker> emptyCopy;

    copyTarget = emptyCopy;

    CHECK_FALSE(copyTarget);
    CHECK(copyDestructionCount == 1);

    auto moveTarget = ownd::MakeStrong<DestructionTracker>(moveDestructionCount);

    ownd::Strong<DestructionTracker> emptyMove;

    moveTarget = std::move(emptyMove);

    CHECK_FALSE(moveTarget);
    CHECK_FALSE(emptyMove);
    CHECK(moveDestructionCount == 1);
}

TEST_CASE("Strong destroys concrete type through non-virtual base") {
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

TEST_CASE("MakeStrong constructs immovable payload in place") {
    auto object = ownd::MakeStrong<ImmovablePayload>(42);

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
