#pragma once
#ifdef _WIN32
#if !defined(_WINSOCK2API_) && !defined(_WINSOCKAPI_)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#endif
#include <array>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <istream>
#include <memory>
#include <mutex>
#include <ostream>
#include <streambuf>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/network/Packet.hpp"

namespace net::minecraft {
class SocketInputStreamBuf : public std::streambuf {
   public:
    explicit SocketInputStreamBuf(SOCKET socket);

   protected:
    int_type underflow() override;
    std::streamsize xsgetn(char* s, std::streamsize count) override;

   private:
    SOCKET socket_ = INVALID_SOCKET;
    std::array<char, 4096> buffer_{};
};

class SocketOutputStreamBuf : public std::streambuf {
   public:
    explicit SocketOutputStreamBuf(SOCKET socket);

   protected:
    int_type overflow(int_type ch) override;
    std::streamsize xsputn(const char* s, std::streamsize count) override;
    int sync() override;

   private:
    bool flushBuffer();
    void sendAll(const char* data, std::size_t length);
    SOCKET socket_ = INVALID_SOCKET;
    std::array<char, 4096> buffer_{};
};

class Connection {
   public:
    static std::atomic<int> readThreadCounter;
    static std::atomic<int> writeThreadCounter;
    static void configureAcceptedSocket(SOCKET socket);
    [[nodiscard]] static int getReadThreadCount() noexcept;
    [[nodiscard]] static int getWriteThreadCount() noexcept;
    Connection(SOCKET socket, std::string name, NetworkHandler& networkHandler);
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    ~Connection();
    void setNetworkHandler(NetworkHandler& networkHandler);
    [[nodiscard]] NetworkHandler* networkHandler() const noexcept;
    [[nodiscard]] bool isOpen() const noexcept;
    [[nodiscard]] const std::string& getAddress() const noexcept;
    [[nodiscard]] std::size_t getDelayedSendQueueSize() const;
    void interrupt();
    void disconnect();
    void disconnect(const std::string& reasonKey, const std::vector<std::string>& args);

    template <typename T, typename... Args>
    void sendPacket(Args&&... args) {
        static_assert(std::is_base_of_v<Packet, T>, "sendPacket requires a Packet type");
        sendPacket(std::make_unique<T>(std::forward<Args>(args)...));
    }

    void sendPacket(std::unique_ptr<Packet> packet);
    void tick();

   private:
    static void ensureWinsock();
    void setSocketOptions();
    [[nodiscard]] std::string formatAddress() const;
    void readLoop();
    void writeLoop();
    void requestDisconnect(std::string reason);
    void shutdownSocket();
    void joinThreads();
    [[nodiscard]] bool hasPendingWrites() const;
    [[nodiscard]] bool readQueueEmpty() const;
    SOCKET socket_ = INVALID_SOCKET;
    std::string name_;
    std::string address_;
    mutable std::atomic<NetworkHandler*> networkHandler_{nullptr};
    std::atomic<bool> open_{true};
    SocketInputStreamBuf inputBuf_;
    std::istream input_;
    SocketOutputStreamBuf outputBuf_;
    std::ostream output_;
    mutable std::mutex readMutex_;
    mutable std::mutex writeMutex_;
    std::condition_variable writeCv_;
    std::deque<std::unique_ptr<Packet>> readQueue_;
    std::deque<std::unique_ptr<Packet>> sendQueue_;
    std::deque<std::unique_ptr<Packet>> delayedSendQueue_;
    std::atomic<std::size_t> sendQueueSize_{0};
    std::thread reader_;
    std::thread writer_;
    int timeoutTicks_ = 0;
    bool disconnectedNotified_ = false;
    std::string disconnectReason_;
    std::vector<std::string> disconnectReasonArgs_;
};
}  // namespace net::minecraft
