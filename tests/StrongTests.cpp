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

    static_assert(std::is_constructible_v<ownd::Strong<BaseObject>, const ownd::Strong<DerivedObject>&>);

    static_assert(std::is_constructible_v<ownd::Strong<const BaseObject>, const ownd::Strong<BaseObject>&>);

    static_assert(!std::is_constructible_v<ownd::Strong<DerivedObject>, const ownd::Strong<BaseObject>&>);

    static_assert(!std::is_constructible_v<ownd::Strong<BaseObject>, const ownd::Strong<const BaseObject>&>);

}

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

TEST_CASE("Strong supports self assignment") {
    bool destroyed = false;

    auto object = ownd::MakeStrong<TestObject>(42, destroyed);
    auto* originalPointer = object.Get();

    object = object;

    CHECK(object.Get() == originalPointer);
    CHECK(object.UseCount() == 1);
    CHECK_FALSE(destroyed);
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
