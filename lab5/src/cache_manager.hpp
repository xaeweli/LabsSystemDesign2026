#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <userver/components/loggable_component_base.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/storages/redis/client.hpp>

USERVER_NAMESPACE_BEGIN

namespace recipe_manager {

class CacheManager final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "cache-manager";

  CacheManager(const components::ComponentConfig& config, const components::ComponentContext& context);

  std::optional<formats::json::Value> Get(const std::string& key) const;
  void Set(const std::string& key, const formats::json::Value& value) const;
  void Invalidate(const std::string& key) const;
  void InvalidateByPattern(const std::string& pattern) const;

 private:
  std::shared_ptr<storages::redis::Client> redis_client_;
  std::chrono::seconds ttl_{std::chrono::seconds(300)};
};

}  // namespace recipe_manager

USERVER_NAMESPACE_END
