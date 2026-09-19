#include <ownd/Ownd.hpp>

#include <doctest/doctest.h>

namespace {

    struct PublicPayload {
        explicit PublicPayload(int value) : Value(value) {}

        int Value;
    };

    struct PublicBase {
        virtual ~PublicBase() = default;

        int Value{42};
    };

    struct PublicDerived final : PublicBase {};

}

TEST_CASE("Ownd public header exposes version information") {
    CHECK(ownd::VERSION_MAJOR == 0);
    CHECK(ownd::VERSION_MINOR == 1);
    CHECK(ownd::VERSION_PATCH == 0);
}

TEST_CASE("Ownd public header exposes Strong and MakeStrong") {
    auto first = ownd::MakeStrong<PublicPayload>(42);

    ownd::Strong<PublicPayload> second = first;

    CHECK(first);
    CHECK(second);

    CHECK(first.Get() == second.Get());
    CHECK(first.UseCount() == 2);
    CHECK(second.UseCount() == 2);

    CHECK(first->Value == 42);
    CHECK(second->Value == 42);
}

TEST_CASE("Ownd public header supports safe conversions") {
    auto derived = ownd::MakeStrong<PublicDerived>();

    ownd::Strong<PublicBase> base = derived;

    ownd::Strong<const PublicBase> readOnly = base;

    CHECK(derived.UseCount() == 3);
    CHECK(base.UseCount() == 3);
    CHECK(readOnly.UseCount() == 3);

    CHECK(derived == base);
    CHECK(base == readOnly);
    CHECK(readOnly->Value == 42);
}

TEST_CASE("Ownd public header supports empty handles") {
    ownd::Strong<int> empty;

    CHECK_FALSE(empty);
    CHECK(empty == nullptr);
    CHECK(empty.UseCount() == 0);
}
