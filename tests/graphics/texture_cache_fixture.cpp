// Asset-free backend fixture for the actual patched SDK methods.
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <future>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace rex {
bool bit_scan_forward(uint64_t bits, uint32_t* index) {
  if (!bits) return false;
  *index = uint32_t(__builtin_ctzll(bits));
  return true;
}
}
struct SharedMemory { static constexpr uint32_t kBufferSize = 0x20000000; };
struct CriticalRegion {
  std::recursive_mutex mutex;
  auto Acquire() { return std::unique_lock<std::recursive_mutex>(mutex); }
};
std::atomic<uint64_t> stat_lookups{0};
struct TextureCache {
  struct Texture { bool scaled; };
  struct TextureKey {
    uint32_t page = 0;
    bool operator==(const TextureKey&) const = default;
    struct Hasher { size_t operator()(TextureKey key) const { return key.page; } };
  };
  struct LookupCacheEntry {
    TextureKey key;
    Texture* texture = nullptr;
    uint64_t epoch = 0;
  };
  static constexpr size_t kLookupCacheSize = 4096;
  CriticalRegion global_critical_region_;
  std::vector<LookupCacheEntry> lookup_cache_;
  std::atomic<uint64_t> lookup_cache_epoch_{1};
  std::array<uint32_t, (SharedMemory::kBufferSize >> 17)> scaled_resolve_pages_{};
  std::array<uint64_t, (SharedMemory::kBufferSize >> 23)> scaled_resolve_pages_l2_{};
  Texture normal{false}, scaled{true};
  bool scale_enabled = true;
  uint32_t allocations = 0;
  bool IsDrawResolutionScaled() const { return scale_enabled; }
  Texture* FindOrCreateTexture(TextureKey key);
  bool IsRangeScaledResolved(uint32_t start, uint32_t length);
  Texture* FindOrCreateTextureUncached(TextureKey key) {
    ++allocations;
    return IsRangeScaledResolved(key.page << 12, 4096) ? &scaled : &normal;
  }
  void SetPage(uint32_t page, bool resolved, bool invalidate = true) {
    // Represents the watch callback's epoch-before-page-update ordering.
    // Caller owns the same global lock as the real callback / GPU writer.
    if (invalidate) lookup_cache_epoch_.fetch_add(1, std::memory_order_acq_rel);
    auto& bits = scaled_resolve_pages_[page >> 5];
    if (resolved) bits |= uint32_t(1) << (page & 31);
    else bits &= ~(uint32_t(1) << (page & 31));
    auto& l2 = scaled_resolve_pages_l2_[page >> 11];
    if (bits) l2 |= uint64_t(1) << ((page >> 5) & 63);
    else l2 &= ~(uint64_t(1) << ((page >> 5) & 63));
  }
};
// SDK_METHODS

void require(bool result, const char* message) {
  if (!result) throw std::runtime_error(message);
}

void RangeChecks() {
  TextureCache cache;
  // Boundaries of both bitmap levels and the physical-memory buffer.
  for (uint32_t page : {0u, 31u, 32u, 2047u, 2048u, 131071u}) {
    auto lock = cache.global_critical_region_.Acquire();
    cache.SetPage(page, true);
    require(cache.IsRangeScaledResolved(page << 12, 1), "marked page missing");
    require(cache.IsRangeScaledResolved((page << 12) + 4095, 1), "page end missing");
    if (page) require(!cache.IsRangeScaledResolved((page << 12) - 1, 1), "previous page leaked");
    cache.SetPage(page, false);
    require(!cache.IsRangeScaledResolved(page << 12, 4096), "cleared page remains resolved");
  }
  require(!cache.IsRangeScaledResolved(0, 0), "zero range resolved");
  require(!cache.IsRangeScaledResolved(SharedMemory::kBufferSize, UINT32_MAX), "past-end range resolved");
  cache.SetPage(131071, true);
  require(cache.IsRangeScaledResolved(SharedMemory::kBufferSize - 1, UINT32_MAX), "clamped range missing");
  cache.scale_enabled = false;
  require(!cache.IsRangeScaledResolved(SharedMemory::kBufferSize - 1, 1), "1x classified as scaled");
}

void LookupChecks() {
  TextureCache cache;
  auto key = TextureCache::TextureKey{32};
  require(cache.FindOrCreateTexture(key) == &cache.normal, "initial classification");
  require(cache.FindOrCreateTexture(key) == &cache.normal, "repeat lookup");
  const bool off = [] {
    auto* f = std::fopen("texture_lookup_cache.off", "rb");
    if (f) std::fclose(f);
    return f != nullptr;
  }();
  require(cache.allocations == (off ? 2u : 1u), "cache toggle/hit behavior");
  {
    auto lock = cache.global_critical_region_.Acquire();
    cache.SetPage(key.page, true);
  }
  require(cache.FindOrCreateTexture(key) == &cache.scaled, "GPU resolve did not invalidate lookup");
  // Hold a CPU write between invalidating the epoch and clearing the bits.
  // Readers must block rather than publish old classification at the new epoch.
  auto lock = cache.global_critical_region_.Acquire();
  cache.lookup_cache_epoch_.fetch_add(1, std::memory_order_acq_rel);
  std::promise<void> started;
  auto ready = started.get_future();
  auto reader = std::async(std::launch::async, [&] {
    started.set_value();
    return cache.FindOrCreateTexture(key);
  });
  ready.wait();
  const bool blocked = reader.wait_for(std::chrono::milliseconds(100)) == std::future_status::timeout;
  cache.SetPage(key.page, false, false);
  lock.unlock();
  auto* result = reader.get();
  require(blocked, "lookup ran during CPU page-map mutation");
  require(result == &cache.normal, "lookup published stale scaled classification");
  require(cache.FindOrCreateTexture(key) == &cache.normal, "stale classification cached");
}

void RangeReaderCheck() {
  TextureCache cache;
  auto lock = cache.global_critical_region_.Acquire();
  cache.SetPage(2048, true);
  std::promise<void> started;
  auto ready = started.get_future();
  auto reader = std::async(std::launch::async, [&] {
    started.set_value();
    return cache.IsRangeScaledResolved(2048 << 12, 4096);
  });
  ready.wait();
  const bool blocked = reader.wait_for(std::chrono::milliseconds(100)) == std::future_status::timeout;
  cache.SetPage(2048, false);
  lock.unlock();
  const bool resolved = reader.get();
  require(blocked, "range reader ran during CPU page-map mutation");
  require(!resolved, "range reader saw stale bits");
}

int main() {
  try {
    RangeChecks();
    LookupChecks();
    RangeReaderCheck();
    std::cout << "Texture classification / synchronization checks passed\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
