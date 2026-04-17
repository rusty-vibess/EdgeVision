#include "RenderServer.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <nlohmann/json.hpp>
#include <jpeglib.h>

using json = nlohmann::json;

// ── file-local helpers ───────────────────────────────────────────────

namespace {

void writeExact(int fd, const void* buf, std::size_t len) {
    auto* p = static_cast<const char*>(buf);
    while (len > 0) {
        auto n = ::write(fd, p, len);
        if (n <= 0) throw std::runtime_error("write failed");
        p += n;
        len -= static_cast<std::size_t>(n);
    }
}

void readExact(int fd, void* buf, std::size_t len) {
    auto* p = static_cast<char*>(buf);
    while (len > 0) {
        auto n = ::read(fd, p, len);
        if (n <= 0) throw std::runtime_error("read: peer closed");
        p += n;
        len -= static_cast<std::size_t>(n);
    }
}

template <typename T>
void writeVal(int fd, T val) {
    writeExact(fd, &val, sizeof(T));
}

RenderPoseRequest parseRequest(int fd) {
    uint32_t jsonLen = 0;
    readExact(fd, &jsonLen, 4);
    if (jsonLen > 1 << 20)
        throw std::runtime_error("json too large");

    std::vector<char> buf(jsonLen);
    readExact(fd, buf.data(), jsonLen);
    auto msg = json::parse(buf.begin(), buf.end());

    RenderPoseRequest req;
    float resX = msg.at("resolution_x");
    float resY = msg.at("resolution_y");
    float fovX = msg.at("fov_x");
    float fovY = msg.at("fov_y");
    req.width  = static_cast<int>(resX);
    req.height = static_cast<int>(resY);
    req.fx = resX / (2.0f * std::tan(fovX / 2.0f));
    req.fy = resY / (2.0f * std::tan(fovY / 2.0f));
    req.cx = resX / 2.0f;
    req.cy = resY / 2.0f;
    req.minimal = msg.contains("minimal") && msg.at("minimal").get<bool>();

    auto poseVec = msg.at("pose").get<std::vector<float>>();
    if (poseVec.size() != 16)
        throw std::runtime_error("pose must have exactly 16 floats");

    // Transpose row-major wire → column-major, then negate columns 1 (Y)
    // and 2 (Z) to convert OpenGL → OpenCV/Colmap. Matches GPS-SLAM
    // remote_viewer.cpp:31-33.
    float col[16];
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            col[c * 4 + r] = poseVec[static_cast<std::size_t>(r * 4 + c)];
    for (int r = 0; r < 4; ++r) {
        col[1 * 4 + r] *= -1.0f;
        col[2 * 4 + r] *= -1.0f;
    }
    // Store back as row-major for Pose4f compatibility.
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            req.pose[r * 4 + c] = col[c * 4 + r];
    return req;
}

std::vector<uint8_t> encodeJpeg(
    const uint8_t* rgb, int width, int height, int quality = 70
) {
    unsigned char* outBuf = nullptr;
    unsigned long outSize = 0;

    jpeg_compress_struct cinfo{};
    jpeg_error_mgr jerr{};
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_mem_dest(&cinfo, &outBuf, &outSize);

    cinfo.image_width  = static_cast<JDIMENSION>(width);
    cinfo.image_height = static_cast<JDIMENSION>(height);
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    while (cinfo.next_scanline < cinfo.image_height) {
        const auto* row = rgb + cinfo.next_scanline * width * 3;
        auto* rowPtr = const_cast<JSAMPROW>(row);
        jpeg_write_scanlines(&cinfo, &rowPtr, 1);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    std::vector<uint8_t> result(outBuf, outBuf + outSize);
    free(outBuf);  // NOLINT — allocated by libjpeg's jpeg_mem_dest
    return result;
}

void sendTrailers(int fd, const RenderPoseRequest& req) {
    // Rotation placeholder (3x3 identity = 9 floats = 36 bytes).
    float rot[9] = {1,0,0, 0,1,0, 0,0,1};
    writeExact(fd, rot, 36);
    // Translation from pose (row-major indices 3, 7, 11).
    float trans[3] = {req.pose[3], req.pose[7], req.pose[11]};
    writeExact(fd, trans, 12);
    // Info string.
    std::string info = "edgevision";
    writeVal<uint32_t>(fd, static_cast<uint32_t>(info.size()));
    writeExact(fd, info.data(), info.size());
    // MVP placeholder (echo the 4x4 pose = 64 bytes).
    writeExact(fd, req.pose, 64);
}

void handleClient(int clientFd, RenderCallback& onRender) {
    while (true) {
        auto req = parseRequest(clientFd);

        const int numPixels = req.width * req.height;
        std::vector<uint8_t> rgb(static_cast<std::size_t>(numPixels) * 3, 0);
        onRender(req, rgb);

        auto jpeg = encodeJpeg(rgb.data(), req.width, req.height);
        writeVal<uint32_t>(clientFd, static_cast<uint32_t>(req.width));
        writeVal<uint32_t>(clientFd, static_cast<uint32_t>(req.height));
        writeVal<uint32_t>(clientFd, static_cast<uint32_t>(jpeg.size()));
        writeExact(clientFd, jpeg.data(), jpeg.size());

        if (!req.minimal) {
            // Non-minimal: 3 extra JPEG images (placeholder black).
            // NOTE: GPS-SLAM sends these as raw RGB (no size prefix).
            // We send JPEG (with size prefix) — a deliberate deviation.
            // Safe because edgevision-mobile always sends minimal=true.
            auto black = encodeJpeg(rgb.data(), req.width, req.height);
            for (int i = 0; i < 3; ++i) {
                writeVal<uint32_t>(clientFd, static_cast<uint32_t>(req.width));
                writeVal<uint32_t>(clientFd, static_cast<uint32_t>(req.height));
                writeVal<uint32_t>(clientFd, static_cast<uint32_t>(black.size()));
                writeExact(clientFd, black.data(), black.size());
            }
        }
        sendTrailers(clientFd, req);
    }
}

} // namespace

// ── RenderServer ─────────────────────────────────────────────────────

void RenderServer::serve(int port, RenderCallback onRender) {
    int serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0)
        throw std::runtime_error("socket() failed");

    int opt = 1;
    if (::setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        std::cerr << "warning: setsockopt(SO_REUSEADDR) failed" << std::endl;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    if (::bind(serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(serverFd);
        throw std::runtime_error("bind() failed on port " + std::to_string(port));
    }
    if (::listen(serverFd, 1) < 0) {
        ::close(serverFd);
        throw std::runtime_error("listen() failed");
    }

    std::cout << "RenderServer listening on port " << port << std::endl;

    while (true) {
        std::cout << "Waiting for client..." << std::endl;
        int clientFd = ::accept(serverFd, nullptr, nullptr);
        if (clientFd < 0) {
            std::cerr << "accept() failed" << std::endl;
            continue;
        }
        std::cout << "Client connected." << std::endl;
        try {
            handleClient(clientFd, onRender);
        } catch (const std::exception& e) {
            std::cout << "Client disconnected: " << e.what() << std::endl;
        }
        ::close(clientFd);
    }
}
