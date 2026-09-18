#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <ownd/ownd.hpp>

TEST_CASE("Ownd exposes version information") {
    CHECK(ownd::VERSION_MAJOR == 0);
    CHECK(ownd::VERSION_MINOR == 1);
    CHECK(ownd::VERSION_PATCH == 0);
}
