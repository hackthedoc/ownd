#include <ownd/Strong.hpp>

#include <doctest/doctest.h>

#include <atomic>
#include <barrier>
#include <cstddef>
#include <thread>
#include <utility>
#include <vector>

namespace {

    constexpr std::size_t ThreadCount = 8;
    constexpr std::size_t IterationCount = 10'000;

    struct ThreadTrackedObject {
        explicit ThreadTrackedObject(std::atomic<int>& destructionCount) : DestructionCount(destructionCount) {}

        ~ThreadTrackedObject() {
            DestructionCount.fetch_add(1, std::memory_order_relaxed);
        }

        std::atomic<int>& DestructionCount;
    };

    struct ThreadBaseObject {
        explicit ThreadBaseObject(std::atomic<int>& destructionCount) : DestructionCount(destructionCount) {}

        ~ThreadBaseObject() {
            DestructionCount.fetch_add(1, std::memory_order_relaxed);
        }

        int Value{42};
        std::atomic<int>& DestructionCount;
    };

    struct ThreadDerivedObject final : ThreadBaseObject {
        ThreadDerivedObject(std::atomic<int>& baseDestructionCount, std::atomic<int>& derivedDestructionCount)
            : ThreadBaseObject(baseDestructionCount)
            , DerivedDestructionCount(derivedDestructionCount)
            {}

        ~ThreadDerivedObject() {
            DerivedDestructionCount.fetch_add(1, std::memory_order_relaxed);
        }

        std::atomic<int>& DerivedDestructionCount;
    };

}

TEST_CASE("Strong supports concurrent ownership copies") {
    std::atomic<int> destructionCount{0};

    auto root = ownd::MakeStrong<ThreadTrackedObject>(destructionCount);

    std::barrier readyBarrier(static_cast<std::ptrdiff_t>(ThreadCount + 1));

    std::barrier releaseBarrier(static_cast<std::ptrdiff_t>(ThreadCount + 1));

    std::vector<std::thread> workers;
    workers.reserve(ThreadCount);

    for (std::size_t i = 0; i < ThreadCount; i++) {
        workers.emplace_back([&root, &readyBarrier, &releaseBarrier]() {
            auto local = root;

            readyBarrier.arrive_and_wait();
            releaseBarrier.arrive_and_wait();
        });
    }

    readyBarrier.arrive_and_wait();

    CHECK(root.UseCount() == ThreadCount + 1);
    CHECK(destructionCount.load(std::memory_order_relaxed) == 0);

    releaseBarrier.arrive_and_wait();

    for (auto& worker : workers)
        worker.join();

    CHECK(root.UseCount() == 1);
    CHECK(destructionCount.load(std::memory_order_relaxed) == 0);

    root.Reset();

    CHECK(destructionCount.load(std::memory_order_relaxed) == 1);
}

TEST_CASE("Strong survives concurrent copy stress") {
    std::atomic<int> destructionCount{0};

    auto root = ownd::MakeStrong<ThreadTrackedObject>(destructionCount);

    std::vector<std::thread> workers;
    workers.reserve(ThreadCount);

    for (std::size_t i = 0; i < ThreadCount; i++) {
        workers.emplace_back([&root]() {
            for (std::size_t j = 0; j < IterationCount; j++) {
                auto first = root;
                auto second = first;
                auto moved = std::move(second);

                first.Reset();
                moved.Reset();
            }
        });
    }

    for (auto& worker : workers)
        worker.join();

    CHECK(root);
    CHECK(root.UseCount() == 1);
    CHECK(destructionCount.load(std::memory_order_relaxed) == 0);

    root.Reset();

    CHECK(destructionCount.load(std::memory_order_relaxed) == 1);
}

TEST_CASE("Concurrent final releases destroy payload once") {
    std::atomic<int> destructionCount{0};

    auto root = ownd::MakeStrong<ThreadTrackedObject>(destructionCount);

    std::vector<ownd::Strong<ThreadTrackedObject>> owners;

    owners.reserve(ThreadCount);

    for (std::size_t i = 0; i < ThreadCount; i++)
        owners.push_back(root);

    CHECK(root.UseCount() == ThreadCount + 1);

    root.Reset();

    std::barrier startBarrier(static_cast<std::ptrdiff_t>(ThreadCount + 1));

    std::vector<std::thread> workers;
    workers.reserve(ThreadCount);

    for (std::size_t i = 0; i < ThreadCount; i++) {
        workers.emplace_back([owner = std::move(owners[i]), &startBarrier]() mutable {
            startBarrier.arrive_and_wait();
            owner.Reset();
        });
    }

    startBarrier.arrive_and_wait();

    for (auto& worker : workers)
        worker.join();

    for (const auto& owner : owners)
        CHECK_FALSE(owner);

    CHECK(destructionCount.load(std::memory_order_relaxed) == 1);
}

TEST_CASE("Strong supports concurrent converting copies") {
    std::atomic<int> baseDestructionCount{0};
    std::atomic<int> derivedDestructionCount{0};
    std::atomic<int> failureCount{0};

    auto root = ownd::MakeStrong<ThreadDerivedObject>(baseDestructionCount, derivedDestructionCount);

    std::vector<std::thread> workers;
    workers.reserve(ThreadCount);

    for (std::size_t i = 0; i < ThreadCount; i++) {
        workers.emplace_back([&root, &failureCount]() {
            for (std::size_t j = 0; j < IterationCount; j++) {
                ownd::Strong<ThreadBaseObject> base = root;

                if (base.Get() == nullptr || base->Value != 42)
                    failureCount.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& worker : workers)
        worker.join();

    CHECK(failureCount.load(std::memory_order_relaxed) == 0);

    CHECK(root.UseCount() == 1);

    root.Reset();

    CHECK(derivedDestructionCount.load(std::memory_order_relaxed) == 1);

    CHECK(baseDestructionCount.load(std::memory_order_relaxed) == 1);
}
