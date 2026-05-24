#include "cache_manager.hpp"

#include <userver/formats/json.hpp>
#include <userver/storages/redis/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace recipe_manager {

CacheManager::CacheManager(const components::ComponentConfig& config, const components::ComponentContext& context)
    : LoggableComponentBase(config, context) {
  auto& redis_component = context.FindComponent<components::Redis>("redis");
  redis_client_ = redis_component.GetClient();
}

std::optional<formats::json::Value> CacheManager::Get(const std::string& key) const {
  auto reply = redis_client_->Get(key);
  if (!reply || reply->IsNil()) {
    return std::nullopt;
  }

  try {
    return formats::json::FromString(reply->GetStdString());
  } catch (const std::exception&) {
    return std::nullopt;
  }
}

void CacheManager::Set(const std::string& key, const formats::json::Value& value) const {
  const auto str = formats::json::ToString(value);
  redis_client_->Set(key, str);
  redis_client_->Expire(key, ttl_);
}

void CacheManager::Invalidate(const std::string& key) const {
  redis_client_->Del(key);
}

void CacheManager::InvalidateByPattern(const std::string& pattern) const {
  redis_client_->Eval(
      R"(
        local keys = redis.call('KEYS', ARGV[1])
        for _, k in ipairs(keys) do
          redis.call('DEL', k)
        end
      )",
      {},
      {pattern}
  );
}

}  // namespace recipe_manager

USERVER_NAMESPACE_END
