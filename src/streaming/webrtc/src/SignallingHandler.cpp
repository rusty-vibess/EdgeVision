#include "streaming/webrtc/SignallingHandler.hpp"

#include <iostream>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace edgevision::streaming::webrtc {

    using WsServer = websocketpp::server<websocketpp::config::asio>;
    using WsHandle = websocketpp::connection_hdl;

    struct SignallingHandler::Impl {
        Impl(std::string host, std::uint16_t port, SignallingCallbacks cb)
            : m_host(std::move(host)),
              m_port(port),
              m_callbacks(std::move(cb)) {
            m_server.clear_access_channels(websocketpp::log::alevel::all);
            m_server.set_error_channels(websocketpp::log::elevel::warn |
                                        websocketpp::log::elevel::rerror |
                                        websocketpp::log::elevel::fatal);
            m_server.init_asio();
            m_server.set_reuse_addr(true);
            m_server.set_open_handler(
                [this](WsHandle hdl) { onOpen(std::move(hdl)); });
            m_server.set_close_handler([this](WsHandle hdl) { onClose(std::move(hdl)); });
            m_server.set_message_handler(
                [this](WsHandle hdl, WsServer::message_ptr msg) {
                    onMessage(std::move(hdl), std::move(msg));
                });
        }

        ~Impl() { stop(); }

        void start() {
            if (m_running.exchange(true)) {
                return;
            }
            m_thread = std::thread([this] {
                try {
                    m_server.listen(m_host, std::to_string(m_port));
                    m_server.start_accept();
                    m_server.run();
                } catch (const std::exception& e) {
                    std::cerr << "signalling server died: " << e.what() << std::endl;
                }
            });
        }

        void stop() {
            if (!m_running.exchange(false)) {
                return;
            }
            try {
                m_server.stop_listening();
            } catch (...) {
            }
            try {
                std::lock_guard<std::mutex> g(m_clientMu);
                if (!m_client.expired()) {
                    m_server.close(m_client, websocketpp::close::status::going_away, "shutdown");
                }
            } catch (...) {
            }
            try {
                m_server.stop();
            } catch (...) {
            }
            if (m_thread.joinable()) {
                m_thread.join();
            }
        }

      private:
        void onOpen(WsHandle hdl) {
            {
                std::lock_guard<std::mutex> g(m_clientMu);
                if (!m_client.expired()) {
                    try {
                        m_server.close(
                            m_client, websocketpp::close::status::going_away,
                            "superseded"
                        );
                    } catch (...) {
                    }
                }
                m_client = hdl;
            }
            if (m_callbacks.onClientConnected) {
                m_callbacks.onClientConnected();
            }
        }

        void onClose(WsHandle hdl) {
            {
                std::lock_guard<std::mutex> g(m_clientMu);
                if (auto strong = m_client.lock()) {
                    if (strong.get() == hdl.lock().get()) {
                        m_client.reset();
                    }
                }
            }
            if (m_callbacks.onClientDisconnected) {
                m_callbacks.onClientDisconnected();
            }
        }

        void onMessage(WsHandle hdl, WsServer::message_ptr msg) {
            const std::string payload = msg->get_payload();
            nlohmann::json j;
            try {
                j = nlohmann::json::parse(payload);
            } catch (const std::exception& e) {
                std::cerr << "signalling parse error: " << e.what() << std::endl;
                return;
            }
            const std::string type = j.value("type", "");
            auto sendFn = [this, hdl](const std::string& s) {
                try {
                    m_server.send(hdl, s, websocketpp::frame::opcode::text);
                } catch (const std::exception& e) {
                    std::cerr << "signalling send error: " << e.what() << std::endl;
                }
            };
            if (type == "offer" && m_callbacks.onOffer) {
                m_callbacks.onOffer(j.value("sdp", std::string{}), sendFn);
            } else if (type == "ice" && m_callbacks.onIce) {
                m_callbacks.onIce(j);
            } else if (type == "hello") {
                // no-op; client identifying itself
            }
        }

        std::string m_host;
        std::uint16_t m_port;
        SignallingCallbacks m_callbacks;
        WsServer m_server;
        std::thread m_thread;
        std::atomic<bool> m_running{false};
        std::mutex m_clientMu;
        WsHandle m_client;
    };

    SignallingHandler::SignallingHandler(
        std::string host, std::uint16_t port, SignallingCallbacks cb
    )
        : m_impl(std::make_unique<Impl>(std::move(host), port, std::move(cb))) {}

    SignallingHandler::~SignallingHandler() = default;

    void SignallingHandler::start() { m_impl->start(); }
    void SignallingHandler::stop() { m_impl->stop(); }

} // namespace edgevision::streaming::webrtc
