#include <ownd/Strong.hpp>

#include <doctest/doctest.h>

#include <stdexcept>
#include <utility>

namespace {

    struct TestObject {
        TestObject(int initialValue, bool& destroyed) : Value(initialValue), Destroyed(destroyed) {}

        ~TestObject() { Destroyed = true; }

        int Value;
        bool& Destroyed;
    };

    struct ThrowingObject {
        explicit ThrowingObject(bool& constructorCalled) {
            constructorCalled = true;

            throw std::runtime_error("construction failed");
        }
    };

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

TEST_CASE("Strong transfers ownership when moved") {
    bool destroyed = false;

    {
        auto original = ownd::MakeStrong<TestObject>(42, destroyed);
        auto moved = std::move(original);

        CHECK_FALSE(original);
        CHECK(original.UseCount() == 0);

        CHECK(moved);
        CHECK(moved.UseCount() == 1);
        CHECK(moved->Value == 42);
        
        CHECK_FALSE(destroyed);
    }

    CHECK(destroyed);
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

    {
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

    CHECK(firstDestroyed);
    CHECK(secondDestroyed);
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

    auto first = ownd::MakeStrong<TestObject>(1, firstDestroyed);
    auto copy = first;
    auto second = ownd::MakeStrong<TestObject>(2, secondDestroyed);

    CHECK(first == copy);
    CHECK_FALSE(first != copy);

    CHECK(first != second);
    CHECK_FALSE(first == second);
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

TEST_CASE("MakeStrong propagates constructor exceptions") {
    bool constructorCalled = false;

    CHECK_THROWS_AS(static_cast<void>(ownd::MakeStrong<ThrowingObject>(constructorCalled)), std::runtime_error);

    CHECK(constructorCalled);
}

TEST_CASE("Strong supports assignment from nullptr") {
    bool destroyed = false;

    auto object = ownd::MakeStrong<TestObject>(42, destroyed);

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
