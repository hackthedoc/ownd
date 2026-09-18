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
