#include "model/frame/FrameStore.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <future>
#include <iostream>
#include <memory>
#include <optional>
#include <source_location>
#include <string_view>
#include <thread>
#include <vector>

namespace {
    using namespace edgevision::model::frame;

    int gFailures = 0;

    void recordFailure(
        std::string_view message,
        const std::source_location& location = std::source_location::current()
    ) {
        std::cerr << location.file_name() << ':' << location.line() << ": " << message << '\n';
        ++gFailures;
    }

    void expectTrue(
        bool value,
        std::string_view message,
        const std::source_location& location = std::source_location::current()
    ) {
        if (!value) {
            recordFailure(message, location);
        }
    }

    template <typename T, typename U>
    void expectEq(
        const T& lhs,
        const U& rhs,
        std::string_view message,
        const std::source_location& location = std::source_location::current()
    ) {
        if (!(lhs == rhs)) {
            recordFailure(message, location);
        }
    }

    FrameBuffer makeBuffer(std::size_t byteCount) {
        auto storage = std::make_shared<std::vector<std::byte>>(byteCount);
        return FrameBuffer(storage->data(), storage->size(), storage);
    }

    Frame makeFrame(FrameId frameId, std::int64_t timestampTicks) {
        Frame frame{};
        frame.frameId = frameId;
        frame.timestamp = FrameTimestamp{timestampTicks};
        frame.rgb.size = ImageSize{2, 2};
        frame.rgb.strideBytes = 6;
        frame.rgb.buffer = makeBuffer(12);
        frame.depth.size = ImageSize{2, 2};
        frame.depth.strideBytes = 4;
        frame.depth.buffer = makeBuffer(8);
        frame.intrinsics = CameraIntrinsics{525.0f, 525.0f, 1.0f, 1.0f};
        return frame;
    }

    void testAcceptedSubmissionPublishesState() {
        FrameStore store(FrameStoreConfig{3, 3});
        const Frame frame = makeFrame(1, 100);

        const FrameSubmissionResult result = store.submitFrame(frame);
        expectTrue(result.accepted(), "submitFrame should accept a valid frame");

        const auto latestFrame = store.getLastFrame();
        expectTrue(latestFrame.has_value(), "accepted frame should become latest");
        expectEq(latestFrame->frameId, FrameId{1}, "latest frame id should match submission");

        const auto lookupFrame = store.getFrame(1);
        expectTrue(lookupFrame.has_value(), "accepted frame should be available by id");

        const auto recentFrames = store.getRecentFrames(5);
        expectEq(recentFrames.size(), std::size_t{1}, "recent frame history should include frame");
        expectEq(recentFrames[0].frameId, FrameId{1}, "recent frame id should match submission");

        const auto nextPacket = store.getNextFramePacket();
        expectTrue(nextPacket.has_value(), "accepted frame should queue a packet");
        expectEq(nextPacket->frameId, FrameId{1}, "queued packet id should match frame");

        const std::optional<FrameLifecycle> lifecycle = store.getFrameLifecycle(1);
        expectTrue(lifecycle.has_value(), "accepted frame should have lifecycle state");
        expectTrue(lifecycle->stored, "accepted lifecycle should record stored");
        expectTrue(lifecycle->queuedForFpga, "accepted lifecycle should record queued_for_fpga");
        expectEq(
            lifecycle->state,
            FrameLifecycleState::QueuedForFpga,
            "accepted lifecycle state should be queued_for_fpga"
        );

        const FrameStoreStatus status = store.getStatus();
        expectEq(status.acceptedFrameCount, std::size_t{1}, "status should count acceptance");
        expectEq(status.rejectedFrameCount, std::size_t{0}, "status should not count rejection");
        expectEq(status.queuedPacketCount, std::size_t{0}, "dequeue should consume queued packet");
        expectTrue(status.latestFrameId.has_value(), "status should expose latest frame id");
        expectEq(*status.latestFrameId, FrameId{1}, "status latest frame id should match");
    }

    void testInvalidFrameDoesNotPoisonStore() {
        FrameStore store(FrameStoreConfig{3, 3});
        expectTrue(
            store.submitFrame(makeFrame(1, 100)).accepted(), "baseline frame should submit"
        );

        Frame invalidFrame = makeFrame(2, 200);
        invalidFrame.depth.buffer = FrameBuffer{};

        const FrameSubmissionResult result = store.submitFrame(invalidFrame);
        expectEq(
            result.code,
            FrameSubmissionCode::MissingDepthBuffer,
            "missing depth should be rejected"
        );

        const auto latestFrame = store.getLastFrame();
        expectTrue(latestFrame.has_value(), "latest frame should remain available");
        expectEq(latestFrame->frameId, FrameId{1}, "invalid frame should not replace latest");
        expectTrue(!store.getFrame(2).has_value(), "invalid frame should not enter lookup");

        const auto nextPacket = store.getNextFramePacket();
        expectTrue(nextPacket.has_value(), "existing packet should remain queued");
        expectEq(nextPacket->frameId, FrameId{1}, "invalid frame should not queue a packet");

        const FrameStoreStatus status = store.getStatus();
        expectEq(status.acceptedFrameCount, std::size_t{1}, "accepted count should remain one");
        expectEq(status.rejectedFrameCount, std::size_t{1}, "rejected count should update");
        expectEq(
            status.lastSubmissionCode,
            FrameSubmissionCode::MissingDepthBuffer,
            "status should expose last rejection"
        );
    }

    void testOrderingValidation() {
        FrameStore store(FrameStoreConfig{3, 3});
        expectTrue(
            store.submitFrame(makeFrame(4, 400)).accepted(), "baseline frame should submit"
        );

        Frame olderIdFrame = makeFrame(3, 500);
        expectEq(
            store.submitFrame(olderIdFrame).code,
            FrameSubmissionCode::FrameIdNotNewer,
            "older frame id should be rejected"
        );

        Frame olderTimestampFrame = makeFrame(5, 300);
        expectEq(
            store.submitFrame(olderTimestampFrame).code,
            FrameSubmissionCode::TimestampNotNewer,
            "older timestamp should be rejected"
        );
    }

    void testDimensionAndIntrinsicsValidation() {
        FrameStore store(FrameStoreConfig{3, 3});

        Frame invalidStrideFrame = makeFrame(2, 200);
        invalidStrideFrame.rgb.strideBytes = 1;
        expectEq(
            store.submitFrame(invalidStrideFrame).code,
            FrameSubmissionCode::InvalidRgbDimensions,
            "invalid RGB stride should be rejected"
        );

        Frame invalidIntrinsicsFrame = makeFrame(3, 300);
        invalidIntrinsicsFrame.intrinsics.fx = 0.0f;
        expectEq(
            store.submitFrame(invalidIntrinsicsFrame).code,
            FrameSubmissionCode::InvalidIntrinsics,
            "invalid intrinsics should be rejected"
        );
    }

    void testPacketHandoffLifecycle() {
        FrameStore store(FrameStoreConfig{4, 4});
        expectTrue(store.submitFrame(makeFrame(1, 100)).accepted(), "frame 1 should submit");
        expectTrue(store.submitFrame(makeFrame(2, 200)).accepted(), "frame 2 should submit");

        const auto firstPacket = store.getNextFramePacket();
        expectTrue(firstPacket.has_value(), "first packet should be available");
        expectEq(firstPacket->frameId, FrameId{1}, "packet queue should be oldest-first");

        expectTrue(!store.markFramePacketSent(2), "cannot mark a non-dequeued packet sent");
        expectTrue(store.markFramePacketSent(1), "first dequeued packet should mark sent");

        const std::optional<FrameLifecycle> sentLifecycle = store.getFrameLifecycle(1);
        expectTrue(sentLifecycle.has_value(), "sent frame lifecycle should be tracked");
        expectTrue(sentLifecycle->sentToFpga, "lifecycle should record sent_to_fpga");
        expectEq(
            sentLifecycle->state,
            FrameLifecycleState::SentToFpga,
            "lifecycle state should be sent_to_fpga"
        );

        const auto secondPacket = store.getNextFramePacket();
        expectTrue(secondPacket.has_value(), "second packet should remain queued");
        expectEq(secondPacket->frameId, FrameId{2}, "second packet should become queue front");

        expectTrue(store.markFramePacketSent(2), "second packet should mark sent");
        expectTrue(!store.getNextFramePacket().has_value(), "packet queue should become empty");
    }

    void testPacketDequeueConsumes() {
        FrameStore store(FrameStoreConfig{4, 4});
        expectTrue(store.submitFrame(makeFrame(1, 100)).accepted(), "frame 1 should submit");
        expectTrue(store.submitFrame(makeFrame(2, 200)).accepted(), "frame 2 should submit");

        const auto firstPacket = store.getNextFramePacket();
        const auto secondPacket = store.getNextFramePacket();

        expectTrue(firstPacket.has_value(), "first packet dequeue should succeed");
        expectTrue(secondPacket.has_value(), "second packet dequeue should succeed");
        expectEq(firstPacket->frameId, FrameId{1}, "first dequeue should consume queue front");
        expectEq(
            secondPacket->frameId, FrameId{2}, "second dequeue should consume next queued packet"
        );
        expectEq(
            store.getStatus().queuedPacketCount,
            std::size_t{0},
            "dequeue should consume queued packets"
        );

        expectTrue(store.markFramePacketSent(1), "first dequeued packet should mark sent");
        expectTrue(store.markFramePacketSent(2), "second dequeued packet should mark sent");
        expectTrue(!store.getNextFramePacket().has_value(), "packet queue should be empty");
    }

    void testBlockingPacketDequeueWaitsForSubmission() {
        FrameStore store(FrameStoreConfig{4, 4});
        std::promise<void> consumerStarted{};
        std::future<void> started = consumerStarted.get_future();
        std::atomic<bool> consumerReturned = false;
        std::optional<FramePacket> dequeuedPacket{};

        std::thread consumer([&store, &consumerStarted, &consumerReturned, &dequeuedPacket]() {
            consumerStarted.set_value();
            dequeuedPacket = store.waitForNextFramePacket();
            consumerReturned.store(true);
        });

        started.wait();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        expectTrue(!consumerReturned.load(), "blocking dequeue should wait before submission");
        expectTrue(store.submitFrame(makeFrame(1, 100)).accepted(), "frame 1 should submit");
        consumer.join();

        expectTrue(consumerReturned.load(), "blocking dequeue should return after submission");
        expectTrue(dequeuedPacket.has_value(), "blocking dequeue should return a packet");
        expectEq(
            dequeuedPacket->frameId, FrameId{1}, "blocking dequeue should observe submitted frame"
        );
        expectEq(
            store.getStatus().queuedPacketCount,
            std::size_t{0},
            "blocking dequeue should consume packet"
        );

        const std::optional<FrameLifecycle> lifecycle = store.getFrameLifecycle(1);
        expectTrue(lifecycle.has_value(), "dequeued packet lifecycle should be tracked");
        expectTrue(
            lifecycle->queuedForFpga && !lifecycle->sentToFpga,
            "blocking dequeue should not mark frame sent"
        );
        expectTrue(store.markFramePacketSent(1), "dequeued packet should mark sent");
    }

    void testBoundedHistoryEvictsSentFrames() {
        FrameStore store(FrameStoreConfig{2, 4});
        expectTrue(store.submitFrame(makeFrame(1, 100)).accepted(), "frame 1 should submit");
        expectTrue(store.getNextFramePacket().has_value(), "frame 1 packet should dequeue");
        expectTrue(store.markFramePacketSent(1), "frame 1 should mark sent");
        expectTrue(store.submitFrame(makeFrame(2, 200)).accepted(), "frame 2 should submit");
        expectTrue(store.getNextFramePacket().has_value(), "frame 2 packet should dequeue");
        expectTrue(store.markFramePacketSent(2), "frame 2 should mark sent");
        expectTrue(store.submitFrame(makeFrame(3, 300)).accepted(), "frame 3 should submit");

        expectTrue(!store.getFrame(1).has_value(), "oldest sent frame should be evicted");
        expectTrue(store.getFrame(2).has_value(), "recent sent frame should remain");
        expectTrue(store.getFrame(3).has_value(), "latest frame should remain");

        const auto recentFrames = store.getRecentFrames(5);
        expectEq(recentFrames.size(), std::size_t{2}, "history should stay bounded");
        expectEq(recentFrames[0].frameId, FrameId{2}, "recent history should preserve order");
        expectEq(recentFrames[1].frameId, FrameId{3}, "latest should be last in recent history");

        const std::optional<FrameLifecycle> evictedLifecycle = store.getFrameLifecycle(1);
        expectTrue(evictedLifecycle.has_value(), "evicted frame lifecycle should remain visible");
        expectTrue(evictedLifecycle->dropped, "evicted frame should be marked dropped");

        const FrameStoreStatus status = store.getStatus();
        expectEq(status.evictedFrameCount, std::size_t{1}, "status should count evictions");
    }

    void testReturnedFrameSurvivesStoreEviction() {
        FrameStore store(FrameStoreConfig{1, 4});
        expectTrue(store.submitFrame(makeFrame(1, 100)).accepted(), "frame 1 should submit");

        const auto returnedFrame = store.getLastFrame();
        expectTrue(returnedFrame.has_value(), "frame 1 should be readable");
        expectTrue(store.getNextFramePacket().has_value(), "frame 1 packet should dequeue");
        expectTrue(store.markFramePacketSent(1), "frame 1 should mark sent");
        expectTrue(store.submitFrame(makeFrame(2, 200)).accepted(), "frame 2 should submit");

        expectTrue(!store.getFrame(1).has_value(), "frame 1 should be evicted from store");
        expectEq(returnedFrame->frameId, FrameId{1}, "returned frame should retain original id");
        expectTrue(
            !returnedFrame->rgb.buffer.empty(), "returned frame should retain RGB buffer lifetime"
        );
        expectEq(
            returnedFrame->rgb.buffer.byteCount,
            std::size_t{12},
            "returned frame should retain RGB buffer size"
        );
    }

    void testHistoryFullProtectsPendingFrames() {
        FrameStore store(FrameStoreConfig{1, 2});
        expectTrue(store.submitFrame(makeFrame(1, 100)).accepted(), "frame 1 should submit");

        const FrameSubmissionResult result = store.submitFrame(makeFrame(2, 200));
        expectEq(
            result.code,
            FrameSubmissionCode::HistoryFull,
            "store should not evict an unsent pending frame"
        );
        expectTrue(
            !store.getFrame(2).has_value(), "history-full rejection should not store frame"
        );
        expectEq(
            store.getStatus().queuedPacketCount,
            std::size_t{1},
            "history-full rejection should not queue packet"
        );
    }

    void testPacketQueueFullRejectsWithoutStateMutation() {
        FrameStore store(FrameStoreConfig{5, 1});
        expectTrue(store.submitFrame(makeFrame(1, 100)).accepted(), "frame 1 should submit");

        const FrameSubmissionResult result = store.submitFrame(makeFrame(2, 200));
        expectEq(
            result.code,
            FrameSubmissionCode::PacketQueueFull,
            "full packet queue should reject frame"
        );
        expectTrue(!store.getFrame(2).has_value(), "queue-full rejection should not store frame");

        const auto latestFrame = store.getLastFrame();
        expectTrue(latestFrame.has_value(), "latest frame should remain available");
        expectEq(
            latestFrame->frameId, FrameId{1}, "queue-full rejection should not replace latest"
        );
    }

    void testConcurrentReadSmoke() {
        FrameStore store(FrameStoreConfig{20, 20});
        std::atomic<bool> done = false;

        std::thread reader([&store, &done]() {
            while (!done.load()) {
                (void)store.getLastFrame();
                (void)store.getRecentFrames(3);
                (void)store.getStatus();
            }
        });

        for (FrameId frameId = 1; frameId <= 5; ++frameId) {
            const std::int64_t timestamp = static_cast<std::int64_t>(frameId) * 100;
            expectTrue(
                store.submitFrame(makeFrame(frameId, timestamp)).accepted(),
                "concurrent smoke frame should submit"
            );
        }

        done.store(true);
        reader.join();

        const auto latestFrame = store.getLastFrame();
        expectTrue(latestFrame.has_value(), "latest frame should exist after concurrent smoke");
        expectEq(latestFrame->frameId, FrameId{5}, "latest frame should be final submitted frame");
    }
} // namespace

int main() {
    testAcceptedSubmissionPublishesState();
    testInvalidFrameDoesNotPoisonStore();
    testOrderingValidation();
    testDimensionAndIntrinsicsValidation();
    testPacketHandoffLifecycle();
    testPacketDequeueConsumes();
    testBlockingPacketDequeueWaitsForSubmission();
    testBoundedHistoryEvictsSentFrames();
    testReturnedFrameSurvivesStoreEviction();
    testHistoryFullProtectsPendingFrames();
    testPacketQueueFullRejectsWithoutStateMutation();
    testConcurrentReadSmoke();

    if (gFailures != 0) {
        std::cerr << "Tests failed: " << gFailures << '\n';
        return 1;
    }

    std::cout << "All tests passed\n";
    return 0;
}
