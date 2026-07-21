#include "net/minecraft/network/Connection.hpp"
#include <algorithm>
#include <chrono>
#include <climits>
#include <cstring>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include "net/minecraft/network/packet/ChunkPackets.hpp"
#include <mstcpip.h>
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
SocketInputStreamBuf::SocketInputStreamBuf(SOCKET socket) : socket_(socket) {
 setg(buffer_.data(), buffer_.data(), buffer_.data());
}
SocketInputStreamBuf::int_type SocketInputStreamBuf::underflow() {
 if(gptr() < egptr()) {
  return traits_type::to_int_type(*gptr());
 }
 if(socket_ == INVALID_SOCKET) {
  return traits_type::eof();
 }
 const int received = ::recv(socket_, buffer_.data(), static_cast<int>(buffer_.size()), 0);
 if(received <= 0) {
  return traits_type::eof();
 }
 setg(buffer_.data(), buffer_.data(), buffer_.data() + received);
 return traits_type::to_int_type(*gptr());
}
std::streamsize SocketInputStreamBuf::xsgetn(char* s, std::streamsize count) {
 std::streamsize total = 0;
 while(count > 0) {
  if(gptr() == egptr()) {
   if(underflow() == traits_type::eof()) {
    break;
   }
  }
  const std::streamsize available = egptr() - gptr();
  const std::streamsize chunk = std::min(available, count);
  std::memcpy(s, gptr(), static_cast<std::size_t>(chunk));
  gbump(static_cast<int>(chunk));
  s += chunk;
  count -= chunk;
  total += chunk;
 }
 return total;
}
SocketOutputStreamBuf::SocketOutputStreamBuf(SOCKET socket) : socket_(socket) {
 setp(buffer_.data(), buffer_.data() + buffer_.size());
}
SocketOutputStreamBuf::int_type SocketOutputStreamBuf::overflow(int_type ch) {
 if(!flushBuffer()) {
  return traits_type::eof();
 }
 if(traits_type::eq_int_type(ch, traits_type::eof())) {
  return traits_type::not_eof(ch);
 }
 *pptr() = traits_type::to_char_type(ch);
 pbump(1);
 return ch;
}
std::streamsize SocketOutputStreamBuf::xsputn(const char* s, std::streamsize count) {
 std::streamsize total = 0;
 while(count > 0) {
  const std::streamsize space = epptr() - pptr();
  if(space == 0) {
   if(!flushBuffer()) {
    break;
   }
   continue;
  }
  const std::streamsize chunk = std::min(space, count);
  std::memcpy(pptr(), s, static_cast<std::size_t>(chunk));
  pbump(static_cast<int>(chunk));
  s += chunk;
  count -= chunk;
  total += chunk;
 }
 return total;
}
int SocketOutputStreamBuf::sync() {
 return flushBuffer() ? 0 : -1;
}
bool SocketOutputStreamBuf::flushBuffer() {
 const std::ptrdiff_t pending = pptr() - pbase();
 if(pending > 0) {
  sendAll(buffer_.data(), static_cast<std::size_t>(pending));
 }
 setp(buffer_.data(), buffer_.data() + buffer_.size());
 return true;
}
void SocketOutputStreamBuf::sendAll(const char* data, std::size_t length) {
 while(length > 0) {
  const int chunk = ::send(
      socket_, data, static_cast<int>(std::min<std::size_t>(length, static_cast<std::size_t>(INT_MAX))), 0);
  if(chunk == SOCKET_ERROR || chunk == 0) {
   throw std::runtime_error("Socket write failed");
  }
  data += chunk;
  length -= static_cast<std::size_t>(chunk);
 }
}
Connection::Connection(SOCKET socket, std::string name, NetworkHandler& networkHandler)
    : socket_(socket),
      name_(std::move(name)),
      inputBuf_(socket_),
      input_(&inputBuf_),
      outputBuf_(socket_),
      output_(&outputBuf_) {
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
 joinThreads();
}
void Connection::disconnect(const std::string& reasonKey, const std::vector<std::string>& args) {
 disconnectReason_ = reasonKey;
 disconnectReasonArgs_ = args;
 requestDisconnect(reasonKey);
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
 if(sendQueueSize_.load(std::memory_order_acquire) > 0x100000) {
  requestDisconnect("disconnect.overflow");
 }
 if(readQueueEmpty()) {
  if(++timeoutTicks_ >= 1200) {
   requestDisconnect("disconnect.timeout");
  }
 } else {
  timeoutTicks_ = 0;
 }
 // Time-boxed adaptive drain. Keep a small minimum so steady-state packet flow does
 // not stall, but honor the wall-clock budget quickly so a burst of expensive packet
 // applies (notably chunk data during local/LAN joins) cannot freeze the game loop.
 constexpr int kMinDrain = 8;
 constexpr int kMaxDrain = 4096;
 constexpr auto kDrainBudget = std::chrono::milliseconds(3);
 const auto drainDeadline = std::chrono::steady_clock::now() + kDrainBudget;
 int applied = 0;
 while(applied < kMaxDrain) {
  std::unique_ptr<Packet> packet;
  {
   std::lock_guard lock(readMutex_);
   if(readQueue_.empty()) {
    break;
   }
   packet = std::move(readQueue_.front());
   readQueue_.pop_front();
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
 if(!isOpen() && readQueueEmpty() && !disconnectedNotified_) {
  disconnectedNotified_ = true;
  if(NetworkHandler* handler = networkHandler()) {
   handler->onDisconnected(disconnectReason_, disconnectReasonArgs_);
  }
 }
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
 ::setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&trueValue), sizeof(trueValue));
}
std::string Connection::formatAddress() const {
 sockaddr_storage storage{};
 int length = sizeof(storage);
 if(::getpeername(socket_, reinterpret_cast<sockaddr*>(&storage), &length) != 0) {
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
void Connection::readLoop() {
 readThreadCounter.fetch_add(1, std::memory_order_acq_rel);
 try {
  while(isOpen()) {
   std::unique_ptr<Packet> packet =
       Packet::read(input_, networkHandler() != nullptr && networkHandler()->isServerSide());
   if(packet == nullptr) {
    requestDisconnect("disconnect.endOfStream");
    break;
   }
   {
    std::lock_guard lock(readMutex_);
    readQueue_.push_back(std::move(packet));
   }
  }
 } catch(const std::exception& error) {
  requestDisconnect(std::string("Internal exception: ") + error.what());
 }
 readThreadCounter.fetch_sub(1, std::memory_order_acq_rel);
}
void Connection::writeLoop() {
 writeThreadCounter.fetch_add(1, std::memory_order_acq_rel);
 try {
  bool preferImmediate = true;
  while(isOpen() || hasPendingWrites()) {
   std::unique_ptr<Packet> packet;
   {
    std::unique_lock lock(writeMutex_);
    writeCv_.wait_for(lock, std::chrono::milliseconds(20), [this]() {
     return !isOpen() || !sendQueue_.empty() || !delayedSendQueue_.empty();
    });
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
   }
   if(packet != nullptr) {
    Packet::write(*packet, output_);
    output_.flush();
    sendQueueSize_.fetch_sub(packet->size() + 1, std::memory_order_acq_rel);
   }
  }
 } catch(const std::exception& error) {
  requestDisconnect(std::string("Internal exception: ") + error.what());
 }
 writeThreadCounter.fetch_sub(1, std::memory_order_acq_rel);
}
void Connection::requestDisconnect(std::string reason) {
 bool expected = true;
 if(!open_.compare_exchange_strong(expected, false, std::memory_order_acq_rel)) {
  return;
 }
 disconnectReason_ = std::move(reason);
 shutdownSocket();
 writeCv_.notify_all();
}
void Connection::shutdownSocket() {
 if(socket_ != INVALID_SOCKET) {
  ::shutdown(socket_, SD_BOTH);
  ::closesocket(socket_);
  socket_ = INVALID_SOCKET;
 }
}
void Connection::joinThreads() {
 const std::thread::id current = std::this_thread::get_id();
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
