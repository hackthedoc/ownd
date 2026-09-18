#include <doctest/doctest.h>

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
