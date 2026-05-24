#include "rate_limiter.hpp"

#include <algorithm>

USERVER_NAMESPACE_BEGIN

namespace recipe_manager {

RateLimiter::RateLimiter(const components::ComponentConfig& config, const components::ComponentContext& context)
    : LoggableComponentBase(config, context) {
  endpoint_limits_["default"] = {1000, std::chrono::seconds(60)};
  endpoint_limits_["recipes-list"] = {100, std::chrono::seconds(60)};
  endpoint_limits_["recipes-search"] = {100, std::chrono::seconds(60)};
  endpoint_limits_["users-create"] = {20, std::chrono::seconds(60)};
  endpoint_limits_["users-by-login"] = {200, std::chrono::seconds(60)};
  endpoint_limits_["users-search"] = {200, std::chrono::seconds(60)};
  endpoint_limits_["recipes-create"] = {50, std::chrono::seconds(60)};
  endpoint_limits_["ingredients-add"] = {60, std::chrono::seconds(60)};
  endpoint_limits_["recipes-ingredients"] = {200, std::chrono::seconds(60)};
  endpoint_limits_["user-recipes"] = {200, std::chrono::seconds(60)};
  endpoint_limits_["favorites-add"] = {30, std::chrono::seconds(60)};
  endpoint_limits_["user-favorites-list"] = {100, std::chrono::seconds(60)};
}

RateLimiter::Result RateLimiter::CheckRateLimit(const std::string& client_key) const {
  const auto now = std::chrono::steady_clock::now();

  const auto& config = [&]() -> const RateLimitConfig& {
    auto it = endpoint_limits_.find(client_key);
    if (it != endpoint_limits_.end()) return it->second;
    return endpoint_limits_.at("default");
  }();

  std::lock_guard lock(mutex_);

  auto& state = states_[client_key];
  const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - state.window_start);

  if (elapsed >= config.window) {
    state.window_start = now;
    state.count = 1;

    Result result;
    result.allowed = true;
    result.limit = config.max_requests;
    result.remaining = config.max_requests - 1;
    result.reset_seconds = static_cast<std::uint64_t>(config.window.count());
    return result;
  }

  state.count++;

  Result result;
  result.allowed = state.count <= config.max_requests;
  result.limit = config.max_requests;
  result.remaining = (state.count <= config.max_requests) ? (config.max_requests - state.count) : 0;
  result.reset_seconds = static_cast<std::uint64_t>(config.window.count()) - static_cast<std::uint64_t>(elapsed.count());

  return result;
}

}  // namespace recipe_manager

USERVER_NAMESPACE_END
