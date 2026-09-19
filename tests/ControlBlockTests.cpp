#include <ownd/detail/ControlBlock.hpp>
#include <ownd/detail/InplaceControlBlock.hpp>

#include <doctest/doctest.h>

namespace {

    class TestControlBlock final : public ownd::detail::ControlBlock {
    public:
        TestControlBlock(int& payloadDestructions, int& blockDestructions) noexcept : m_PayloadDestructions(payloadDestructions), m_BlockDestructions(blockDestructions) {}

        ~TestControlBlock() override { m_BlockDestructions++; }

    private:
        void DestroyPayload() noexcept override { m_PayloadDestructions++; }

        void DestroyBlock() noexcept override { delete this; }

    private:
        int& m_PayloadDestructions;
        int& m_BlockDestructions;
    };

    struct LifetimeProbe {
        LifetimeProbe(int initialValue, bool& destroyed) : Value(initialValue), Destroyed(destroyed) {}

        ~LifetimeProbe() { Destroyed = true; }

        int Value;
        bool& Destroyed;
    };

}

TEST_CASE("ControlBlock starts with one strong owner") {
    int payloadDestructions = 0;
    int blockDestructions = 0;

    TestControlBlock* controlBlock = new TestControlBlock(payloadDestructions, blockDestructions);

    CHECK(controlBlock->UseCount() == 1);

    controlBlock->ReleaseStrong();

    CHECK(payloadDestructions == 1);
    CHECK(blockDestructions == 1);
}

TEST_CASE("ControlBlock destroys its payload on final release") {
    int payloadDestructions = 0;
    int blockDestructions = 0;

    TestControlBlock* controlBlock = new TestControlBlock(payloadDestructions, blockDestructions);

    controlBlock->AddStrong();

    CHECK(controlBlock->UseCount() == 2);

    controlBlock->ReleaseStrong();

    CHECK(controlBlock->UseCount() == 1);
    CHECK(payloadDestructions == 0);
    CHECK(blockDestructions == 0);

    controlBlock->ReleaseStrong();

    CHECK(payloadDestructions == 1);
    CHECK(blockDestructions == 1);
}

TEST_CASE("InplaceControlBlock constructs and destroys its object") {
    bool destroyed = false;

    auto* block = new ownd::detail::InplaceControlBlock<LifetimeProbe>(42, destroyed);

    const auto* constBlock = block;

    CHECK(block->Get() != nullptr);
    CHECK(block->Get()->Value == 42);

    CHECK(constBlock->Get() != nullptr);
    CHECK(constBlock->Get()->Value == 42);

    CHECK_FALSE(destroyed);

    block->ReleaseStrong();

    CHECK(destroyed);
}
