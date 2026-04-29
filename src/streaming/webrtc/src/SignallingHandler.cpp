#include "streaming/webrtc/SignallingHandler.hpp"

#include <iostream>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace edgevision::streaming::webrtc {

    using WsServer = websocketpp::server<websocketpp::config::asio>;
    using WsHandle = websocketpp::connection_hdl;

    struct SignallingHandler::Impl {
        Impl(std::string host, std::uint16_t port, SignallingCallbacks cb)
            : m_host(std::move(host)), m_port(port), m_callbacks(std::move(cb)) {
            configureLogging();
            m_server.init_asio();
            m_server.set_reuse_addr(true);
            m_server.set_open_handler([this](WsHandle hdl) { onOpen(std::move(hdl)); });
            m_server.set_close_handler([this](WsHandle hdl) { onClose(std::move(hdl)); });
            m_server.set_message_handler([this](WsHandle hdl, WsServer::message_ptr msg) {
                onMessage(std::move(hdl), std::move(msg));
            });
        }

        ~Impl() {
            stop();
        }

        void start() {
            if (m_running.exchange(true)) {
                return;
            }
            m_stopping.store(false);
            configureLogging();
            m_thread = std::thread([this] {
                try {
                    m_server.listen(m_host, std::to_string(m_port));
                    m_server.start_accept();
                    m_server.run();
                } catch (const std::exception& e) {
                    if (!m_stopping.load()) {
                        std::cerr << "signalling server died: " << e.what() << std::endl;
                    }
                }
            });
        }

        void stop() {
            if (!m_running.exchange(false)) {
                return;
            }
            m_stopping.store(true);
            suppressLoggingForShutdown();
            try {
                m_server.stop_listening();
            } catch (...) {}
            closeActiveClient("shutdown");
            if (m_thread.joinable()) {
                m_thread.join();
            }
        }

        void closeActiveClient(const std::string& reason) {
            WsHandle activeClient;
            {
                std::lock_guard<std::mutex> g(m_clientMu);
                if (m_client.expired()) {
                    return;
                }
                activeClient = m_client;
            }

            m_server.get_io_service().post([this, activeClient, reason] {
                {
                    std::lock_guard<std::mutex> g(m_clientMu);
                    if (!isActiveClientLocked(activeClient)) {
                        return;
                    }
                }

                try {
                    m_server.close(activeClient, websocketpp::close::status::going_away, reason);
                } catch (...) {}
            });
        }

      private:
        void configureLogging() {
            m_server.clear_access_channels(websocketpp::log::alevel::all);
            m_server.set_error_channels(
                websocketpp::log::elevel::warn | websocketpp::log::elevel::rerror
                | websocketpp::log::elevel::fatal
            );
        }

        void suppressLoggingForShutdown() {
            m_server.clear_access_channels(websocketpp::log::alevel::all);
            m_server.clear_error_channels(websocketpp::log::elevel::all);
        }

        [[nodiscard]] bool isActiveClientLocked(const WsHandle& hdl) {
            auto active = m_client.lock();
            auto current = hdl.lock();
            return active && current && active.get() == current.get();
        }

        [[nodiscard]] bool isActiveClient(const WsHandle& hdl) {
            std::lock_guard<std::mutex> g(m_clientMu);
            return isActiveClientLocked(hdl);
        }

        [[nodiscard]] SendFunc makeSendFunc(WsHandle hdl) {
            return [this, hdl = std::move(hdl)](const std::string& payload) {
                m_server.get_io_service().post([this, hdl, payload] {
                    {
                        std::lock_guard<std::mutex> g(m_clientMu);
                        if (!isActiveClientLocked(hdl)) {
                            return;
                        }
                    }

                    try {
                        m_server.send(hdl, payload, websocketpp::frame::opcode::text);
                    } catch (const std::exception& e) {
                        if (!m_stopping.load()) {
                            std::cerr << "signalling send error: " << e.what() << std::endl;
                        }
                    }
                });
            };
        }

        void onOpen(WsHandle hdl) {
            WsHandle previousClient;
            bool closePreviousClient = false;
            {
                std::lock_guard<std::mutex> g(m_clientMu);
                if (!m_client.expired()) {
                    previousClient = m_client;
                    closePreviousClient = true;
                }
                m_client = hdl;
            }

            if (closePreviousClient) {
                try {
                    m_server.close(
                        previousClient, websocketpp::close::status::going_away, "superseded"
                    );
                } catch (...) {}
            }

            if (m_callbacks.onClientConnected) {
                m_server.get_io_service().post([this, hdl] {
                    if (!isActiveClient(hdl)) {
                        std::cerr << "signalling startup skipped for inactive client" << std::endl;
                        return;
                    }

                    m_callbacks.onClientConnected(makeSendFunc(hdl));
                });
            }
        }

        void onClose(WsHandle hdl) {
            bool closedActiveClient = false;
            int remoteCloseCode = -1;
            std::string remoteCloseReason;
            try {
                auto connection = m_server.get_con_from_hdl(hdl);
                remoteCloseCode = static_cast<int>(connection->get_remote_close_code());
                remoteCloseReason = connection->get_remote_close_reason();
            } catch (...) {}

            {
                std::lock_guard<std::mutex> g(m_clientMu);
                if (isActiveClientLocked(hdl)) {
                    m_client.reset();
                    closedActiveClient = true;
                }
            }

            std::cerr << "signalling websocket closed "
                      << (closedActiveClient ? "active" : "stale") << " code=" << remoteCloseCode
                      << " reason='" << remoteCloseReason << "'\n";
            if (closedActiveClient && m_callbacks.onClientDisconnected) {
                m_callbacks.onClientDisconnected();
            }
        }

        void onMessage(WsHandle hdl, WsServer::message_ptr msg) {
            if (!isActiveClient(hdl)) {
                std::cerr << "ignoring signalling message from inactive client" << std::endl;
                return;
            }

            const std::string payload = msg->get_payload();
            nlohmann::json j;
            try {
                j = nlohmann::json::parse(payload);
            } catch (const std::exception& e) {
                std::cerr << "signalling parse error: " << e.what() << std::endl;
                return;
            }
            const std::string type = j.value("type", "");
            if (type == "answer" && m_callbacks.onAnswer) {
                m_callbacks.onAnswer(j.value("sdp", std::string{}));
            } else if (type == "ice" && m_callbacks.onIce) {
                m_callbacks.onIce(j);
            } else if (type == "hello") {
                // no-op; client identifying itself
            } else if (type == "offer") {
                std::cerr << "ignoring inbound offer in server-offer mode\n";
            }
        }

        std::string m_host;
        std::uint16_t m_port;
        SignallingCallbacks m_callbacks;
        WsServer m_server;
        std::thread m_thread;
        std::atomic<bool> m_running{false};
        std::atomic<bool> m_stopping{false};
        std::mutex m_clientMu;
        WsHandle m_client;
    };

    SignallingHandler::SignallingHandler(
        std::string host,
        std::uint16_t port,
        SignallingCallbacks cb
    )
        : m_impl(std::make_unique<Impl>(std::move(host), port, std::move(cb))) {}

    SignallingHandler::~SignallingHandler() = default;

    void SignallingHandler::start() {
        m_impl->start();
    }

    void SignallingHandler::stop() {
        m_impl->stop();
    }

    void SignallingHandler::closeActiveClient(const std::string& reason) {
        m_impl->closeActiveClient(reason);
    }

} // namespace edgevision::streaming::webrtc
