#include "net/minecraft/network/Connection.hpp"
#include <mstcpip.h>
#include <algorithm>
#include <chrono>
#include <climits>
#include <mutex>
#include <stdexcept>
#include "net/minecraft/network/packet/ChunkPackets.hpp"
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
#include "net/minecraft/util/concurrent/ThreadNames.hpp"
namespace {
constexpr std::size_t kMaxReadQueueBytes = 0x2000000;
constexpr std::size_t kReadQueueHighWater = kMaxReadQueueBytes / 2;
constexpr std::size_t kReadQueueLowWater = kMaxReadQueueBytes / 4;
constexpr std::size_t kMaxSendQueueBytes = 0x100000;
} // namespace
namespace net::minecraft {
std::atomic<int> Connection::readThreadCounter{0};
std::atomic<int> Connection::writeThreadCounter{0};
void Connection::configureAcceptedSocket(SOCKET socket) {
 const BOOL trueValue = TRUE;
 ::setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&trueValue), sizeof(trueValue));
 const int recvTimeoutMs = 30'000;
 const int sendTimeoutMs = 30'000;
 ::setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&recvTimeoutMs), sizeof(recvTimeoutMs));
 ::setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&sendTimeoutMs), sizeof(sendTimeoutMs));
 const int trafficClass = 24;
 ::setsockopt(socket, IPPROTO_IP, IP_TOS, reinterpret_cast<const char*>(&trafficClass), sizeof(trafficClass));
}
int Connection::getReadThreadCount() noexcept {
 return readThreadCounter.load(std::memory_order_acquire);
}
int Connection::getWriteThreadCount() noexcept {
 return writeThreadCounter.load(std::memory_order_acquire);
}
Connection::Connection(SOCKET socket, std::string name, NetworkHandler& networkHandler)
    : socket_(socket), name_(std::move(name)) {
 ensureWinsock();
 setSocketOptions();
 address_ = formatAddress();
 setNetworkHandler(networkHandler);
 reader_ = std::thread([this]() { readLoop(); });
 writer_ = std::thread([this]() { writeLoop(); });
}
Connection::~Connection() {
 disconnect();
 joinThreads();
}
void Connection::setNetworkHandler(NetworkHandler& networkHandler) {
 networkHandler_.store(&networkHandler, std::memory_order_release);
}
NetworkHandler* Connection::networkHandler() const noexcept {
 return networkHandler_.load(std::memory_order_acquire);
}
bool Connection::isOpen() const noexcept {
 return open_.load(std::memory_order_acquire);
}
const std::string& Connection::getAddress() const noexcept {
 return address_;
}
std::size_t Connection::getDelayedSendQueueSize() const {
 std::lock_guard lock(writeMutex_);
 return delayedSendQueue_.size();
}
void Connection::interrupt() {
 writeCv_.notify_all();
}
void Connection::disconnect() {
 requestDisconnect("disconnect.closed");
}
void Connection::disconnect(const std::string& reasonKey, const std::vector<std::string>& args) {
 {
  std::lock_guard lock(disconnectMutex_);
  disconnectReason_ = reasonKey;
  disconnectReasonArgs_ = args;
 }
 requestDisconnect(reasonKey);
}
void Connection::setDrainLimit(const DrainLimit& limit) {
 externalDrainLimit_ = limit;
}
void Connection::clearDrainLimit() {
 externalDrainLimit_.reset();
}
void Connection::sendPacket(std::unique_ptr<Packet> packet) {
 if(packet == nullptr || !isOpen()) {
  return;
 }
 if(auto* chunkPacket = dynamic_cast<ChunkDataS2CPacket*>(packet.get())) {
  chunkPacket->compressForSend();
 }
 {
  std::lock_guard lock(writeMutex_);
  sendQueueSize_ += packet->size() + 1;
  if(packet->worldPacket) {
   delayedSendQueue_.push_back(std::move(packet));
  } else {
   sendQueue_.push_back(std::move(packet));
  }
 }
 writeCv_.notify_one();
}
void Connection::tick() {
 if(sendQueueSize_.load(std::memory_order_acquire) > kMaxSendQueueBytes) {
  requestDisconnect("disconnect.overflow");
 }
 if(readQueueEmpty()) {
  if(++timeoutTicks_ >= 1200) {
   requestDisconnect("disconnect.timeout");
  }
 } else {
  timeoutTicks_ = 0;
 }
 // Drain budget is opt-in: only a Java-MP join or a hosted server sets a
 // drain limit (ConnectionListener sets one per tick). Everything else — the
 // integrated/singleplayer path never goes through Connection at all, and a
 // plain client connection is budgeted by its handler — drains the whole
 // backlog up to kMaxDrain with no wall-clock cap, so a burst of expensive
 // applies (chunk data during a local/LAN join) clears in a single tick.
 constexpr int kMinDrain = 8;
 constexpr int kMaxDrain = 4096;
 constexpr std::chrono::milliseconds kFallbackDrainBudget = std::chrono::milliseconds(3);
 int maxDrain =
     externalDrainLimit_.has_value() ? std::min(externalDrainLimit_->maxPackets, kMaxDrain) : kMaxDrain;
 // Always impose a wall-clock budget: a fallback with no external limit must not
 // drain the whole backlog (up to kMaxDrain) synchronously in one main-thread
 // tick, which stalls the frame on a join/stream burst.
 const std::chrono::steady_clock::time_point drainDeadline =
     externalDrainLimit_.has_value()
         ? externalDrainLimit_->deadline
         : std::chrono::steady_clock::now() + kFallbackDrainBudget;
 int applied = 0;
 while(applied < maxDrain) {
  std::unique_ptr<Packet> packet;
  {
   std::lock_guard lock(readMutex_);
   if(readQueue_.empty()) {
    break;
   }
   const std::size_t packetBytes = readQueue_.front()->size() + 1;
   packet = std::move(readQueue_.front());
   readQueue_.pop_front();
   readQueueSize_ -= std::min(readQueueSize_.load(std::memory_order_acquire), packetBytes);
  }
  if(packet != nullptr) {
   if(NetworkHandler* handler = networkHandler()) {
    packet->apply(*handler);
   }
  }
  if(++applied >= kMinDrain && std::chrono::steady_clock::now() >= drainDeadline) {
   break;
  }
 }
 {
  std::lock_guard lock(readMutex_);
  if(!readStats_.empty()) {
   Packet::mergeReadStats(readStats_);
   readStats_.clear();
  }
 }
 if(!isOpen() && readQueueEmpty() && !disconnectedNotified_) {
  disconnectedNotified_ = true;
  std::string reason;
  std::vector<std::string> args;
  {
   std::lock_guard lock(disconnectMutex_);
   reason = disconnectReason_;
   args = disconnectReasonArgs_;
  }
  if(NetworkHandler* handler = networkHandler()) {
   handler->onDisconnected(reason, args);
  }
 }
 // Wake the reader if backpressure paused it above the low-water mark.
 readCv_.notify_all();
}
void Connection::ensureWinsock() {
 static std::once_flag once;
 std::call_once(once, []() {
  WSADATA data{};
  const int result = ::WSAStartup(MAKEWORD(2, 2), &data);
  if(result != 0) {
   throw std::runtime_error("WSAStartup failed");
  }
 });
}
void Connection::setSocketOptions() {
 const BOOL trueValue = TRUE;
 const SOCKET socket = socket_.load(std::memory_order_acquire);
 ::setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&trueValue), sizeof(trueValue));
 // Backstop for a peer that stops reading: a blocked send() must not stall the
 // writer thread forever (WI-10 async teardown). joinThreads() force-closes the
 // socket to unblock a pending send immediately; this timeout bounds the worst
 // case for sockets that were never given a graceful close.
 const int sendTimeoutMs = 1000;
 ::setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&sendTimeoutMs), sizeof(sendTimeoutMs));
}
std::string Connection::formatPeerAddress(SOCKET socket) {
 sockaddr_storage storage{};
 int length = sizeof(storage);
 if(::getpeername(socket, reinterpret_cast<sockaddr*>(&storage), &length) != 0) {
  return "unknown";
 }
 char host[NI_MAXHOST]{};
 char service[NI_MAXSERV]{};
 if(::getnameinfo(reinterpret_cast<sockaddr*>(&storage),
                  length,
                  host,
                  sizeof(host),
                  service,
                  sizeof(service),
                  NI_NUMERICHOST | NI_NUMERICSERV) != 0) {
  return "unknown";
 }
 return std::string(host) + ":" + service;
}
std::string Connection::formatAddress() const {
 return formatPeerAddress(socket_.load(std::memory_order_acquire));
}
void Connection::readLoop() {
 util::concurrent::tl_domain = util::concurrent::Domain::NetIo;
 readThreadCounter.fetch_add(1, std::memory_order_acq_rel);
 try {
  while(isOpen()) {
   for(;;) {
    const std::uint8_t* src = readBuffer_.data() + readPos_;
    const std::uint8_t* end = readBuffer_.data() + readBuffer_.size();
    if(src >= end) {
     break;
    }
    std::unique_ptr<Packet> packet;
    try {
     packet = Packet::read(src, end, networkHandler() != nullptr && networkHandler()->isServerSide());
    } catch(const packetio::PacketUnderflow&) {
     break;
    }
    if(packet == nullptr) {
     requestDisconnect("disconnect.endOfStream");
     readBuffer_.clear();
     readPos_ = 0;
     break;
    }
    const int rawId = packet->rawId();
    const int size = static_cast<int>(packet->size());
    {
     // Backpressure: hold a large inbound backlog (e.g. a burst of chunk packets
     // during a join) at the high-water mark until the game thread drains it below
     // the low-water mark, rather than buffering without bound or killing the
     // connection. requestDisconnect() wakes the wait so teardown is not blocked.
     std::unique_lock lock(readMutex_);
     readCv_.wait(lock, [this] { return !isOpen() || readQueueSize_.load(std::memory_order_acquire) <= kReadQueueHighWater; });
     if(!isOpen()) {
      break;
     }
     readQueue_.push_back(std::move(packet));
     readQueueSize_.fetch_add(static_cast<std::size_t>(size) + 1, std::memory_order_acq_rel);
     readStats_.emplace_back(rawId, size);
    }
    readPos_ = static_cast<std::size_t>(src - readBuffer_.data());
   }
   readBuffer_.erase(readBuffer_.begin(), readBuffer_.begin() + readPos_);
   readPos_ = 0;
   if(!isOpen()) {
    break;
   }
   std::uint8_t chunk[4096];
   const int received = ::recv(socket_.load(std::memory_order_acquire),
                               reinterpret_cast<char*>(chunk),
                               static_cast<int>(sizeof(chunk)),
                               0);
   if(received <= 0) {
    requestDisconnect("disconnect.endOfStream");
    break;
   }
   readBuffer_.insert(readBuffer_.end(), chunk, chunk + received);
  }
 } catch(const std::exception& error) {
  requestDisconnect(std::string("Internal exception: ") + error.what());
 }
 readThreadCounter.fetch_sub(1, std::memory_order_acq_rel);
}
void Connection::writeLoop() {
 util::concurrent::tl_domain = util::concurrent::Domain::NetIo;
 writeThreadCounter.fetch_add(1, std::memory_order_acq_rel);
 try {
  bool preferImmediate = true;
  std::optional<std::chrono::steady_clock::time_point> closeGrace;
  while(isOpen() || hasPendingWrites()) {
   std::vector<std::unique_ptr<Packet>> batch;
   {
    std::unique_lock lock(writeMutex_);
    if(!isOpen() && !closeGrace.has_value()) {
     closeGrace = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
    }
    writeCv_.wait_for(lock, std::chrono::milliseconds(20), [this]() {
     return !isOpen() || !sendQueue_.empty() || !delayedSendQueue_.empty();
    });
    if(!isOpen() && closeGrace.has_value() && std::chrono::steady_clock::now() >= *closeGrace) {
     sendQueue_.clear();
     delayedSendQueue_.clear();
     sendQueueSize_.store(0, std::memory_order_release);
     break;
    }
    while(!sendQueue_.empty() || !delayedSendQueue_.empty()) {
     std::unique_ptr<Packet> packet;
     if(!sendQueue_.empty() && !delayedSendQueue_.empty()) {
      if(preferImmediate) {
       packet = std::move(sendQueue_.front());
       sendQueue_.pop_front();
      } else {
       packet = std::move(delayedSendQueue_.front());
       delayedSendQueue_.pop_front();
      }
      preferImmediate = !preferImmediate;
     } else if(!sendQueue_.empty()) {
      packet = std::move(sendQueue_.front());
      sendQueue_.pop_front();
      preferImmediate = false;
     } else if(!delayedSendQueue_.empty()) {
      packet = std::move(delayedSendQueue_.front());
      delayedSendQueue_.pop_front();
      preferImmediate = true;
     }
     if(packet != nullptr) {
      batch.push_back(std::move(packet));
     }
    }
   }
   if(!batch.empty()) {
    for(auto& packet : batch) {
     if(packet != nullptr) {
      writeScratch_.resize(packet->size() + 1);
      std::uint8_t* dest = writeScratch_.data();
      std::uint8_t* const end = dest + writeScratch_.size();
      Packet::write(*packet, dest, end);
      sendAll(writeScratch_.data(), static_cast<std::size_t>(dest - writeScratch_.data()));
      sendQueueSize_.fetch_sub(packet->size() + 1, std::memory_order_acq_rel);
     }
    }
   }
  }
 } catch(const std::exception& error) {
  requestDisconnect(std::string("Internal exception: ") + error.what());
 }
 shutdownSocket();
 writeThreadCounter.fetch_sub(1, std::memory_order_acq_rel);
}
void Connection::sendAll(const std::uint8_t* data, std::size_t length) {
 const SOCKET socket = socket_.load(std::memory_order_acquire);
 while(length > 0) {
  const int chunk = ::send(
      socket, reinterpret_cast<const char*>(data),
      static_cast<int>(std::min<std::size_t>(length, static_cast<std::size_t>(INT_MAX))), 0);
  if(chunk == SOCKET_ERROR || chunk == 0) {
   throw std::runtime_error("Socket write failed");
  }
  data += chunk;
  length -= static_cast<std::size_t>(chunk);
 }
}
void Connection::requestDisconnect(std::string reason) {
 bool expected = true;
 if(!open_.compare_exchange_strong(expected, false, std::memory_order_acq_rel)) {
  return;
 }
 {
  std::lock_guard lock(disconnectMutex_);
  disconnectReason_ = std::move(reason);
 }
 const SOCKET socket = socket_.load(std::memory_order_acquire);
 if(socket != INVALID_SOCKET) {
  ::shutdown(socket, SD_RECEIVE);
 }
 writeCv_.notify_all();
 readCv_.notify_all();
}
void Connection::shutdownSocket() {
 const SOCKET socket = socket_.exchange(INVALID_SOCKET);
 if(socket != INVALID_SOCKET) {
  ::shutdown(socket, SD_BOTH);
  ::closesocket(socket);
 }
}
void Connection::joinThreads() {
 const std::thread::id current = std::this_thread::get_id();
 const std::chrono::steady_clock::time_point deadline =
     std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
 while((reader_.joinable() && reader_.get_id() != current) ||
       (writer_.joinable() && writer_.get_id() != current)) {
  if(std::chrono::steady_clock::now() >= deadline) {
   // shutdown(SD_BOTH) does not reliably unblock a blocked send() on Windows;
   // force-close the socket so any pending recv/send returns immediately. The
   // exchange keeps this idempotent with shutdownSocket().
   const SOCKET socket = socket_.exchange(INVALID_SOCKET);
   if(socket != INVALID_SOCKET) {
    ::shutdown(socket, SD_BOTH);
    ::closesocket(socket);
   }
   break;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
 }
 if(reader_.joinable() && reader_.get_id() != current) {
  reader_.join();
 }
 if(writer_.joinable() && writer_.get_id() != current) {
  writer_.join();
 }
}
bool Connection::hasPendingWrites() const {
 std::lock_guard lock(writeMutex_);
 return !sendQueue_.empty() || !delayedSendQueue_.empty();
}
bool Connection::readQueueEmpty() const {
 std::lock_guard lock(readMutex_);
 return readQueue_.empty();
}
} // namespace net::minecraft
