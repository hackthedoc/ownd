# Ownd

Ownd is an experimental C++23 header-only library for shared object ownership.

It provides a deliberately small ownership API built around:

- `ownd::Strong<T>`: a strong shared-ownership handle
- `ownd::MakeStrong<T>(...)`: in-place object construction
- an atomic reference-counted control block

Ownd is primarily an educational project for exploring ownership models, object lifetime, generic programming, and concurrency in modern C++.

> Ownd is not intended to replace `std::shared_ptr` in production code. The standard library implementation is mature, portable, and extensively tested.

## Status

Current version: **0.1.0**

The initial release focuses on the smallest coherent shared-ownership model. Features such as weak references, custom deleters, raw-pointer adoption, and arrays are intentionally outside the scope of v0.1.0.

## Basic usage

```cpp
#include <ownd/Ownd.hpp>

#include <iostream>

struct Widget {
    explicit Widget(int value) : Value(value) {}

    int Value;
};

int main() {
    auto first = ownd::MakeStrong<Widget>(42);
    ownd::Strong<Widget> second = first;

    std::cout << first->Value << '\n';
    std::cout << first.UseCount() << '\n';
}
```

The object is destroyed when its final `Strong` owner releases it.

## Type conversions

`Strong<U>` can convert to `Strong<T>` when `U*` is implicitly convertible to `T*`.

```cpp
#include <ownd/Ownd.hpp>

struct Base {
    virtual ~Base() = default;
};

struct Derived final : Base {};

int main() {
    auto derived = ownd::MakeStrong<Derived>();

    ownd::Strong<Base> base = derived;
    ownd::Strong<const Base> readOnly = base;
}
```

Unsafe conversions, such as `Base` to `Derived`, unrelated types, or removing `const`, are rejected at compile time.

## Public API

### `ownd::Strong<T>`

```cpp
ownd::Strong<T> pointer;
ownd::Strong<T> empty = nullptr;
```

Important operations:

```cpp
pointer.Get();
pointer.UseCount();
pointer.Reset();
pointer.Swap(other);

*pointer;
pointer->member;

if (pointer) {
    // The handle contains a pointer.
}
```

`Strong<T>` is copyable and movable. Copying adds another strong owner, while moving transfers ownership without changing the total owner count.

### `ownd::MakeStrong<T>(...)`

```cpp
auto pointer = ownd::MakeStrong<T>(constructorArguments...);
```

`MakeStrong` allocates one in-place control block containing the managed object and constructs the object with the forwarded arguments.

## Thread safety

The strong reference count is atomic.

It is safe for different `Strong` instances that share the same control block to be copied, moved, reset, or destroyed concurrently.

```cpp
auto root = ownd::MakeStrong<int>(42);

ownd::Strong<int> first = root;
ownd::Strong<int> second = root;

// Separate handles may now be used by separate threads.
```

The following are not provided automatically:

- Concurrent mutation of the same `Strong` object
- Synchronization of the managed payload

If multiple threads modify the managed object, the user must provide the required synchronization.

## About `UseCount()`

`UseCount()` returns a snapshot of the number of strong owners.

```cpp
auto pointer = ownd::MakeStrong<int>(42);
auto count = pointer.UseCount();
```

In concurrent code, that value may become outdated immediately. It is useful for observation, diagnostics, and tests, but should not be used as a synchronization primitive.

Ownd intentionally does not provide a `Unique()` operation: a named predicate makes a racy read look like a settled fact, which invites checking it and then acting as if nothing could have changed since. Where the check is genuinely meaningful in your own single-threaded or externally-synchronized code, `UseCount() == 1` says the same thing without the false sense of safety.

## Building and testing

Configure the project with tests enabled:

```powershell
xmake f -c -p mingw -a x86_64 -m debug --tests=y
```

Build and run the test suite:

```powershell
xmake
xmake run ownd_tests
```

## Installing

Install Ownd to a local prefix:

```powershell
xmake install -o build/install owned
```

This installs the public headers and package metadata required by xmake consumers. Package-repository distribution (so a consumer doesn't need a local path) isn't set up yet.

## Limitations of v0.1.0

The following features are not currently supported:

- Weak ownership
- Raw-pointer adoption
- Custom deleters
- Custom allocators
- Arrays
- Aliasing constructors
- Pointer-cast helpers
- A stable ABI guarantee

Keeping the first release narrow makes its ownership rules easier to understand, test, and evolve.

## Roadmap

Possible future work includes:

- `Weak<T>` and control-block weak counts
- Ownership cast helpers
- Custom deleters and allocators
- Additional sanitizer and compiler coverage
- Continuous integration
- Package-manager integration
- Performance and allocation benchmarks

These are candidates, not commitments for a specific release.

## License

Ownd is available under the MIT License. See [LICENSE](LICENSE) for details.
