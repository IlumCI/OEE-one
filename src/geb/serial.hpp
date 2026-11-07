// Minimal binary serialization for checkpoints (little-endian host format; a
// checkpoint is only read back by the same build on the same platform).
#pragma once

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace oee::geb {

class Writer {
 public:
  template <class T>
  void put(const T& v) {
    static_assert(std::is_trivially_copyable_v<T>);
    const auto* p = reinterpret_cast<const unsigned char*>(&v);
    buf_.insert(buf_.end(), p, p + sizeof(T));
  }
  template <class T>
  void put_vec(const std::vector<T>& v) {
    static_assert(std::is_trivially_copyable_v<T>);
    put<std::uint64_t>(v.size());
    if (v.empty()) return;
    const auto* p = reinterpret_cast<const unsigned char*>(v.data());
    buf_.insert(buf_.end(), p, p + v.size() * sizeof(T));
  }
  void put_str(const std::string& s) {
    put<std::uint64_t>(s.size());
    buf_.insert(buf_.end(), s.begin(), s.end());
  }
  const std::vector<unsigned char>& bytes() const { return buf_; }

 private:
  std::vector<unsigned char> buf_;
};

class Reader {
 public:
  Reader(const unsigned char* data, std::size_t size) : p_(data), end_(data + size) {}
  template <class T>
  T get() {
    static_assert(std::is_trivially_copyable_v<T>);
    need(sizeof(T));
    T v;
    std::memcpy(&v, p_, sizeof(T));
    p_ += sizeof(T);
    return v;
  }
  template <class T>
  std::vector<T> get_vec() {
    const auto n = get<std::uint64_t>();
    need(n * sizeof(T));
    std::vector<T> v(n);
    if (n) std::memcpy(v.data(), p_, n * sizeof(T));
    p_ += n * sizeof(T);
    return v;
  }
  std::string get_str() {
    const auto n = get<std::uint64_t>();
    need(n);
    std::string s(reinterpret_cast<const char*>(p_), n);
    p_ += n;
    return s;
  }
  bool done() const { return p_ == end_; }

 private:
  void need(std::size_t n) {
    if (static_cast<std::size_t>(end_ - p_) < n) throw std::runtime_error("checkpoint truncated");
  }
  const unsigned char* p_;
  const unsigned char* end_;
};

inline std::uint64_t fnv1a(const unsigned char* p, std::size_t n) {
  std::uint64_t h = 1469598103934665603ULL;
  for (std::size_t i = 0; i < n; ++i) {
    h ^= p[i];
    h *= 1099511628211ULL;
  }
  return h;
}

}  // namespace oee::geb
