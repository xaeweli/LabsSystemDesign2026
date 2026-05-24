#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

#include <userver/components/loggable_component_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace recipe_manager {

struct RateLimitConfig {
  std::uint64_t max_requests{0};
  std::chrono::seconds window{std::chrono::seconds(1)};
};

struct SlidingWindowState {
  std::chrono::steady_clock::time_point window_start;
  std::uint64_t count{0};
};

class RateLimiter final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "rate-limiter";

  RateLimiter(const components::ComponentConfig& config, const components::ComponentContext& context);

  struct Result {
    bool allowed{false};
    std::uint64_t limit{0};
    std::uint64_t remaining{0};
    std::uint64_t reset_seconds{0};
  };

  Result CheckRateLimit(const std::string& client_key) const;

 private:
  std::unordered_map<std::string, RateLimitConfig> endpoint_limits_;

  mutable std::mutex mutex_;
  mutable std::unordered_map<std::string, SlidingWindowState> states_;
};

}  // namespace recipe_manager

USERVER_NAMESPACE_END
