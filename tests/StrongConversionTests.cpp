#include <ownd/Strong.hpp>

#include <doctest/doctest.h>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace {

    struct BaseObject {
        virtual ~BaseObject() = default;

        int Value{42};
    };

    struct DerivedObject final : BaseObject {
        explicit DerivedObject(bool& destroyed) : Destroyed(destroyed) {}

        ~DerivedObject() override { Destroyed = true; }

        bool& Destroyed;
    };

}

using StrongInt = ownd::Strong<int>;

static_assert(
    std::is_constructible_v<ownd::Strong<BaseObject>, const ownd::Strong<DerivedObject>&>
);

static_assert(
    std::is_constructible_v<ownd::Strong<const BaseObject>, const ownd::Strong<BaseObject>&>
);

static_assert(
    !std::is_constructible_v<ownd::Strong<DerivedObject>, const ownd::Strong<BaseObject>&>
);

static_assert(
    !std::is_constructible_v<ownd::Strong<BaseObject>, const ownd::Strong<const BaseObject>&>
);

static_assert(
    std::is_nothrow_default_constructible_v<StrongInt>
);

static_assert(
    std::is_nothrow_copy_constructible_v<StrongInt>
);

static_assert(
    std::is_nothrow_move_constructible_v<StrongInt>
);

static_assert(
    std::is_nothrow_copy_assignable_v<StrongInt>
);

static_assert(
    std::is_nothrow_move_assignable_v<StrongInt>
);

static_assert(
    std::is_nothrow_constructible_v<ownd::Strong<BaseObject>, const ownd::Strong<DerivedObject>&>
);

static_assert(
    std::is_nothrow_constructible_v<ownd::Strong<BaseObject>, ownd::Strong<DerivedObject>&&>
);

static_assert(
    std::is_nothrow_assignable_v<ownd::Strong<BaseObject>&, const ownd::Strong<DerivedObject>&>
);

static_assert(
    std::is_nothrow_assignable_v<ownd::Strong<BaseObject>&, ownd::Strong<DerivedObject>&&>
);

static_assert(noexcept(
    std::declval<StrongInt&>().Reset()
));

static_assert(noexcept(
    std::declval<StrongInt&>().Swap(std::declval<StrongInt&>())
));

static_assert(noexcept(
    std::declval<const StrongInt&>().Get()
));

static_assert(noexcept(
    std::declval<const StrongInt&>().UseCount()
));

static_assert(noexcept(
    std::declval<const StrongInt&>() == std::declval<const StrongInt&>()
));

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

    base.Reset();

    CHECK(destroyed);
}

TEST_CASE("Strong supports derived to base copy assignment") {
    bool destroyed = false;

    auto derived = ownd::MakeStrong<DerivedObject>(destroyed);

    ownd::Strong<BaseObject> base;
    base = derived;

    CHECK(derived.UseCount() == 2);
    CHECK(base.UseCount() == 2);
    CHECK(base.Get() == derived.Get());
    CHECK_FALSE(destroyed);

    base.Reset();

    CHECK(derived.UseCount() == 1);
    CHECK_FALSE(destroyed);

    derived.Reset();

    CHECK(destroyed);
}

TEST_CASE("Strong supports derived to base move assignment") {
    bool destroyed = false;

    auto derived = ownd::MakeStrong<DerivedObject>(destroyed);

    DerivedObject* originalPointer = derived.Get();

    ownd::Strong<BaseObject> base;
    base = std::move(derived);

    CHECK_FALSE(derived);
    CHECK(derived.UseCount() == 0);

    CHECK(base.Get() == originalPointer);
    CHECK(base.UseCount() == 1);
    CHECK_FALSE(destroyed);

    base.Reset();

    CHECK(destroyed);
}

TEST_CASE("Strong supports conversion to const") {
    bool destroyed = false;

    auto mutableObject = ownd::MakeStrong<DerivedObject>(destroyed);

    ownd::Strong<const DerivedObject> readOnly = mutableObject;

    CHECK(mutableObject.UseCount() == 2);
    CHECK(readOnly.UseCount() == 2);
    CHECK(readOnly->Value == 42);

    mutableObject.Reset();

    CHECK(readOnly.UseCount() == 1);
    CHECK_FALSE(destroyed);

    readOnly.Reset();

    CHECK(destroyed);
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
