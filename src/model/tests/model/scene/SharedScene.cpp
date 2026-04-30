#include "model/scene/SharedScene.hpp"

#include <atomic>
#include <chrono>
#include <exception>
#include <future>
#include <iostream>
#include <memory>
#include <optional>
#include <source_location>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "scene/testing/SharedSceneTestAccess.hpp"

namespace {
    using edgevision::config::SceneReadPolicy;
    using namespace edgevision::model::scene;
    using edgevision::model::scene::testing::SharedSceneTestAccess;
    using namespace std::chrono_literals;

    static_assert(
        !std::is_invocable_v<decltype(&SharedScene::publish), SharedScene*, SceneReadAccess&>,
        "read access must not publish scene versions"
    );

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

    template <typename Exception, typename Function>
    void expectThrows(
        Function function,
        std::string_view message,
        const std::source_location& location = std::source_location::current()
    ) {
        try {
            function();
            recordFailure(message, location);
        } catch (const Exception&) {
        } catch (const std::exception& exception) {
            std::cerr << location.file_name() << ':' << location.line()
                      << ": unexpected exception: " << exception.what() << '\n';
            ++gFailures;
        } catch (...) {
            recordFailure("unexpected non-standard exception", location);
        }
    }

    template <typename Predicate>
    bool waitUntil(Predicate predicate, std::chrono::milliseconds timeout = 1s) {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            if (predicate()) {
                return true;
            }

            std::this_thread::sleep_for(1ms);
        }

        return predicate();
    }

    bool waitForWaitingWriter(const SharedScene& scene, std::chrono::milliseconds timeout = 1s) {
        return waitUntil(
            [&scene]() { return SharedSceneTestAccess::lockState(scene).waitingWriterCount > 0; },
            timeout
        );
    }

    void expectBlockedReadGetsPriorityAheadOfWaitingWriter(SceneReadPolicy readPolicy) {
        SharedScene scene{readPolicy};
        std::promise<void> readerStarted{};
        std::promise<void> writerStarted{};
        std::promise<void> releaseReader{};
        std::future<void> readerStartedFuture = readerStarted.get_future();
        std::future<void> writerStartedFuture = writerStarted.get_future();
        std::shared_future<void> releaseReaderSignal = releaseReader.get_future().share();
        std::atomic<bool> readerAcquired = false;
        std::atomic<bool> writerAcquired = false;
        std::thread reader{};
        std::thread writer{};

        {
            auto initialWrite = scene.write();
            reader =
                std::thread([&scene, &readerStarted, &releaseReaderSignal, &readerAcquired]() {
                    readerStarted.set_value();
                    auto readAccess = scene.read();
                    readerAcquired.store(true);
                    releaseReaderSignal.wait();
                });
            writer = std::thread([&scene, &writerStarted, &writerAcquired]() {
                writerStarted.set_value();
                auto writeAccess = scene.write();
                writerAcquired.store(true);
            });

            readerStartedFuture.wait();
            writerStartedFuture.wait();

            expectTrue(
                !waitUntil([&readerAcquired]() { return readerAcquired.load(); }, 20ms),
                "blocked read should wait for active write"
            );
            expectTrue(
                !waitUntil([&writerAcquired]() { return writerAcquired.load(); }, 20ms),
                "waiting writer should wait for active write"
            );
        }

        expectTrue(
            waitUntil([&readerAcquired]() { return readerAcquired.load(); }),
            "blocked read should acquire after active write releases"
        );
        expectTrue(
            !writerAcquired.load(), "blocked read should be granted before the next writer"
        );

        releaseReader.set_value();
        reader.join();

        expectTrue(
            waitUntil([&writerAcquired]() { return writerAcquired.load(); }),
            "waiting writer should acquire after the promoted read releases"
        );
        writer.join();
    }

    void testRepeatedSharedSceneConstructionReleasesScene() {
        for (int i = 0; i < 10; ++i) {
            auto scene = std::make_unique<SharedScene>();
            expectTrue(scene != nullptr, "shared scene allocation should succeed");
        }
    }

    void testReadAccessCanCoexist() {
        SharedScene scene{};
        std::promise<void> writerStarted{};
        std::future<void> started = writerStarted.get_future();
        std::atomic<bool> writerAcquired = false;
        std::thread writer{};

        {
            auto firstRead = scene.read();
            auto secondRead = scene.read();

            expectEq(firstRead.version(), SceneVersionId{0}, "first read should be granted");
            expectEq(secondRead.version(), SceneVersionId{0}, "second read should be granted");

            writer = std::thread([&scene, &writerStarted, &writerAcquired]() {
                writerStarted.set_value();
                auto writeAccess = scene.write();
                writerAcquired.store(true);
            });

            started.wait();
            expectTrue(
                !waitUntil([&writerAcquired]() { return writerAcquired.load(); }, 20ms),
                "write access should wait for active readers"
            );
        }

        expectTrue(
            waitUntil([&writerAcquired]() { return writerAcquired.load(); }),
            "writer should acquire after readers release"
        );
        writer.join();
    }

    void testWriteAccessIsExclusive() {
        SharedScene scene{};
        std::promise<void> readerStarted{};
        std::promise<void> writerStarted{};
        std::promise<void> releaseReader{};
        std::future<void> readerStartedFuture = readerStarted.get_future();
        std::future<void> writerStartedFuture = writerStarted.get_future();
        std::shared_future<void> releaseReaderSignal = releaseReader.get_future().share();
        std::atomic<bool> readerAcquired = false;
        std::atomic<bool> writerAcquired = false;
        std::thread reader{};
        std::thread writer{};

        {
            auto initialWrite = scene.write();
            reader =
                std::thread([&scene, &readerStarted, &releaseReaderSignal, &readerAcquired]() {
                    readerStarted.set_value();
                    auto readAccess = scene.read();
                    readerAcquired.store(true);
                    releaseReaderSignal.wait();
                });
            writer = std::thread([&scene, &writerStarted, &writerAcquired]() {
                writerStarted.set_value();
                auto writeAccess = scene.write();
                writerAcquired.store(true);
            });

            readerStartedFuture.wait();
            writerStartedFuture.wait();
            expectTrue(
                !waitUntil([&readerAcquired]() { return readerAcquired.load(); }, 20ms),
                "read access should not overlap active write access"
            );
            expectTrue(
                !waitUntil([&writerAcquired]() { return writerAcquired.load(); }, 20ms),
                "write access should not overlap active write access"
            );
        }

        expectTrue(
            waitUntil([&readerAcquired]() { return readerAcquired.load(); }),
            "blocked read should acquire after active write releases"
        );
        expectTrue(!writerAcquired.load(), "waiting writer should remain blocked behind read");

        releaseReader.set_value();
        reader.join();
        expectTrue(
            waitUntil([&writerAcquired]() { return writerAcquired.load(); }),
            "blocked writer should acquire after reader releases"
        );
        writer.join();
    }

    void testBlockingReadWaitsForWriteRelease() {
        SharedScene scene{};
        std::promise<void> consumerStarted{};
        std::future<void> started = consumerStarted.get_future();
        std::atomic<bool> consumerReturned = false;
        std::optional<SceneReadAccess> readAccess{};
        std::thread consumer{};

        {
            auto writeAccess = scene.write();
            consumer = std::thread([&scene, &consumerStarted, &consumerReturned, &readAccess]() {
                consumerStarted.set_value();
                readAccess = scene.read();
                consumerReturned.store(true);
            });

            started.wait();
            expectTrue(
                !waitUntil([&consumerReturned]() { return consumerReturned.load(); }, 20ms),
                "blocking read access should wait for write release"
            );
        }

        consumer.join();
        expectTrue(consumerReturned.load(), "blocking read access should return after release");
        expectTrue(readAccess.has_value(), "blocking read access should produce a handle");
    }

    void testBlockingWriteWaitsForReadRelease() {
        SharedScene scene{};
        std::promise<void> consumerStarted{};
        std::future<void> started = consumerStarted.get_future();
        std::atomic<bool> consumerReturned = false;
        std::optional<SceneWriteAccess> writeAccess{};
        std::thread consumer{};

        {
            auto readAccess = scene.read();
            consumer = std::thread([&scene, &consumerStarted, &consumerReturned, &writeAccess]() {
                consumerStarted.set_value();
                writeAccess = scene.write();
                consumerReturned.store(true);
            });

            started.wait();
            expectTrue(
                !waitUntil([&consumerReturned]() { return consumerReturned.load(); }, 20ms),
                "blocking write access should wait for read release"
            );
        }

        consumer.join();
        expectTrue(consumerReturned.load(), "blocking write access should return after release");
        expectTrue(writeAccess.has_value(), "blocking write access should produce a handle");
    }

    void testBlockedReadGetsPriorityAheadOfWaitingWriter() {
        expectBlockedReadGetsPriorityAheadOfWaitingWriter(SceneReadPolicy::Greedy);
        expectBlockedReadGetsPriorityAheadOfWaitingWriter(SceneReadPolicy::Balanced);
    }

    void testGreedyPolicyAllowsFreshReadersPastWaitingWriter() {
        SharedScene scene{SceneReadPolicy::Greedy};
        std::promise<void> secondReaderStarted{};
        std::promise<void> writerStarted{};
        std::promise<void> releaseSecondReader{};
        std::future<void> secondReaderStartedFuture = secondReaderStarted.get_future();
        std::future<void> writerStartedFuture = writerStarted.get_future();
        std::shared_future<void> releaseSecondReaderSignal =
            releaseSecondReader.get_future().share();
        std::atomic<bool> secondReaderAcquired = false;
        std::atomic<bool> writerAcquired = false;
        std::thread secondReader{};
        std::thread writer{};

        {
            auto firstReader = scene.read();
            writer = std::thread([&scene, &writerStarted, &writerAcquired]() {
                writerStarted.set_value();
                auto writeAccess = scene.write();
                writerAcquired.store(true);
            });

            writerStartedFuture.wait();
            expectTrue(
                waitForWaitingWriter(scene),
                "writer should become queued behind the existing reader"
            );
            secondReader = std::thread([&scene,
                                        &secondReaderStarted,
                                        &releaseSecondReaderSignal,
                                        &secondReaderAcquired]() {
                secondReaderStarted.set_value();
                auto readAccess = scene.read();
                secondReaderAcquired.store(true);
                releaseSecondReaderSignal.wait();
            });

            secondReaderStartedFuture.wait();

            expectTrue(
                waitUntil([&secondReaderAcquired]() { return secondReaderAcquired.load(); }),
                "greedy policy should allow a fresh reader past a waiting writer"
            );
            expectTrue(
                !writerAcquired.load(),
                "waiting writer should remain blocked while readers still hold access"
            );
        }

        expectTrue(
            !writerAcquired.load(),
            "waiting writer should stay blocked until the fresh reader releases"
        );

        releaseSecondReader.set_value();
        secondReader.join();

        expectTrue(
            waitUntil([&writerAcquired]() { return writerAcquired.load(); }),
            "waiting writer should acquire after all readers release"
        );
        writer.join();
    }

    void testBalancedPolicyBlocksFreshReadersBehindWaitingWriter() {
        SharedScene scene{SceneReadPolicy::Balanced};
        std::promise<void> secondReaderStarted{};
        std::promise<void> writerStarted{};
        std::promise<void> releaseWriter{};
        std::future<void> secondReaderStartedFuture = secondReaderStarted.get_future();
        std::future<void> writerStartedFuture = writerStarted.get_future();
        std::shared_future<void> releaseWriterSignal = releaseWriter.get_future().share();
        std::atomic<bool> secondReaderAcquired = false;
        std::atomic<bool> writerAcquired = false;
        std::thread secondReader{};
        std::thread writer{};

        {
            auto firstReader = scene.read();
            writer =
                std::thread([&scene, &writerStarted, &releaseWriterSignal, &writerAcquired]() {
                    writerStarted.set_value();
                    auto writeAccess = scene.write();
                    writerAcquired.store(true);
                    releaseWriterSignal.wait();
                });

            writerStartedFuture.wait();
            expectTrue(
                waitForWaitingWriter(scene),
                "writer should become queued behind the existing reader"
            );
            secondReader = std::thread([&scene, &secondReaderStarted, &secondReaderAcquired]() {
                secondReaderStarted.set_value();
                auto readAccess = scene.read();
                secondReaderAcquired.store(true);
            });
            secondReaderStartedFuture.wait();

            expectTrue(
                !waitUntil(
                    [&secondReaderAcquired]() { return secondReaderAcquired.load(); }, 20ms
                ),
                "balanced policy should not let a fresh reader bypass a waiting writer"
            );
        }

        expectTrue(
            waitUntil([&writerAcquired]() { return writerAcquired.load(); }),
            "waiting writer should acquire once existing readers release"
        );
        expectTrue(
            !secondReaderAcquired.load(),
            "new reader should remain blocked until the waiting writer finishes"
        );

        releaseWriter.set_value();
        writer.join();
        expectTrue(
            waitUntil([&secondReaderAcquired]() { return secondReaderAcquired.load(); }),
            "blocked reader should acquire after the waiting writer releases"
        );
        secondReader.join();
    }

    void testManyReadersShareAccessAndBlockWriter() {
        SharedScene scene{};
        constexpr int readerCount = 8;

        std::promise<void> releaseReaders{};
        std::promise<void> writerStarted{};
        std::shared_future<void> releaseSignal = releaseReaders.get_future().share();
        std::future<void> writerStartedFuture = writerStarted.get_future();
        std::atomic<int> acquiredReaders = 0;
        std::atomic<bool> writerAcquired = false;
        std::vector<std::thread> readers{};
        readers.reserve(readerCount);
        std::thread writer{};

        for (int index = 0; index < readerCount; ++index) {
            readers.emplace_back([&scene, releaseSignal, &acquiredReaders]() {
                auto access = scene.read();
                acquiredReaders.fetch_add(1);
                releaseSignal.wait();
            });
        }

        expectTrue(
            waitUntil([&acquiredReaders]() { return acquiredReaders.load() == readerCount; }),
            "all concurrent readers should acquire access"
        );
        writer = std::thread([&scene, &writerStarted, &writerAcquired]() {
            writerStarted.set_value();
            auto writeAccess = scene.write();
            writerAcquired.store(true);
        });
        writerStartedFuture.wait();
        expectTrue(
            !waitUntil([&writerAcquired]() { return writerAcquired.load(); }, 20ms),
            "writer should wait for all readers"
        );

        releaseReaders.set_value();
        for (std::thread& reader : readers) {
            reader.join();
        }

        expectTrue(
            waitUntil([&writerAcquired]() { return writerAcquired.load(); }),
            "writer should acquire after readers release"
        );
        writer.join();
    }

    void testPublishRequiresOwningWriteAccess() {
        SharedScene scene{};
        SharedScene otherScene{};

        {
            auto foreignWrite = otherScene.write();
            expectThrows<std::invalid_argument>(
                [&scene, &foreignWrite]() {
                    [[maybe_unused]] const SceneVersionId ignoredVersion =
                        scene.publish(foreignWrite);
                },
                "publish should reject write access from another scene"
            );
        }

        expectEq(scene.version(), SceneVersionId{0}, "foreign publish should not change version");
        expectEq(
            otherScene.version(),
            SceneVersionId{0},
            "rejected foreign publish should not change foreign version"
        );

        {
            auto writeAccess = scene.write();
            auto movedWriteAccess = std::move(writeAccess);
            expectThrows<std::invalid_argument>(
                [&scene, &writeAccess]() {
                    [[maybe_unused]] const SceneVersionId ignoredVersion =
                        scene.publish(writeAccess);
                },
                "publish should reject moved-from write access"
            );
            expectEq(
                movedWriteAccess.version(),
                SceneVersionId{0},
                "valid moved-to write access should remain usable"
            );
        }
    }

    void testPublishUpdatesVersionForHandleAndFutureReaders() {
        SharedScene scene{};
        expectEq(scene.version(), SceneVersionId{0}, "new scene should start at version 0");

        {
            auto writeAccess = scene.write();
            const SceneVersionId publishedVersion = scene.publish(writeAccess);
            expectEq(publishedVersion, SceneVersionId{1}, "publish should increment version");
            expectEq(
                writeAccess.version(),
                SceneVersionId{1},
                "write handle should observe published version"
            );
        }

        expectEq(scene.version(), SceneVersionId{1}, "published version should persist");

        auto readAccess = scene.read();
        expectEq(
            readAccess.version(),
            SceneVersionId{1},
            "future readers should observe published version"
        );
    }

    void testInfiniTamSceneReferenceIsCentralAndStable() {
        SharedScene scene{};
        const InfiniTamScene* sceneAddress = nullptr;

        {
            auto readAccess = scene.read();
            sceneAddress = &readAccess.scene();
            expectTrue(sceneAddress != nullptr, "read access should expose scene");
        }

        {
            auto writeAccess = scene.write();
            expectTrue(
                sceneAddress == &writeAccess.scene(),
                "read and write access should expose same central InfiniTAM scene"
            );
        }
    }

    void testMovedFromHandlesRejectLeaseOperations() {
        SharedScene scene{};

        {
            auto readAccess = scene.read();
            auto movedReadAccess = std::move(readAccess);
            const InfiniTamScene& movedReadScene = movedReadAccess.scene();
            expectTrue(
                &movedReadScene == &movedReadAccess.scene(), "moved read access should stay valid"
            );
            expectThrows<std::logic_error>(
                [&readAccess]() {
                    [[maybe_unused]] const InfiniTamScene& ignoredScene = readAccess.scene();
                },
                "moved-from read access should reject scene access"
            );
            expectThrows<std::logic_error>(
                [&readAccess]() {
                    [[maybe_unused]] const SceneVersionId ignoredVersion = readAccess.version();
                },
                "moved-from read access should reject version access"
            );
        }

        {
            auto writeAccess = scene.write();
            auto movedWriteAccess = std::move(writeAccess);
            const InfiniTamScene& movedWriteScene = movedWriteAccess.scene();
            expectTrue(
                &movedWriteScene == &movedWriteAccess.scene(),
                "moved write access should stay valid"
            );
            expectThrows<std::logic_error>(
                [&writeAccess]() {
                    [[maybe_unused]] InfiniTamScene& ignoredScene = writeAccess.scene();
                },
                "moved-from write access should reject scene access"
            );
            expectThrows<std::logic_error>(
                [&writeAccess]() {
                    [[maybe_unused]] const SceneVersionId ignoredVersion = writeAccess.version();
                },
                "moved-from write access should reject version access"
            );
        }
    }
} // namespace

int main() {
    testRepeatedSharedSceneConstructionReleasesScene();
    testReadAccessCanCoexist();
    testWriteAccessIsExclusive();
    testBlockingReadWaitsForWriteRelease();
    testBlockingWriteWaitsForReadRelease();
    testBlockedReadGetsPriorityAheadOfWaitingWriter();
    testGreedyPolicyAllowsFreshReadersPastWaitingWriter();
    testBalancedPolicyBlocksFreshReadersBehindWaitingWriter();
    testManyReadersShareAccessAndBlockWriter();
    testPublishRequiresOwningWriteAccess();
    testPublishUpdatesVersionForHandleAndFutureReaders();
    testInfiniTamSceneReferenceIsCentralAndStable();
    testMovedFromHandlesRejectLeaseOperations();

    if (gFailures != 0) {
        std::cerr << "Tests failed: " << gFailures << '\n';
        return 1;
    }

    std::cout << "All tests passed\n";
    return 0;
}
