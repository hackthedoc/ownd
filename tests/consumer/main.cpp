#include "ownd/Strong.hpp"
#include <ownd/Ownd.hpp>

namespace {
    
    struct Widget {
        explicit Widget(int value) : Value(value) {}

        int Value;
    };

}

int main() {
    static_assert(ownd::VERSION_MAJOR == 0);
    static_assert(ownd::VERSION_MINOR == 1);
    static_assert(ownd::VERSION_PATCH == 0);

    auto first = ownd::MakeStrong<Widget>(42);
    ownd::Strong<Widget> second = first;
    ownd::Strong<const Widget> readOnly = second;

    if (!first || !second || !readOnly)
        return 1;

    if (first->Value != 42)
        return 2;

    if (second->Value != 42)
        return 3;

    if (readOnly->Value != 42)
        return 4;

    if (first.Get() != second.Get())
        return 5;

    if (first.UseCount() != 3)
        return 6;

    if (second.UseCount() != 3)
        return 7;

    if (readOnly.UseCount() != 3)
        return 8;

    first.Reset();

    if (second.UseCount() != 2)
        return 9;

    return 0;
}
