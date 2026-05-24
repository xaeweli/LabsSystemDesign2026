#pragma once

#include <functional>
#include <string>
#include <string_view>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>

#include "cache_manager.hpp"
#include "rate_limiter.hpp"
#include "recipe_storage.hpp"

USERVER_NAMESPACE_BEGIN

namespace recipe_manager {

class HandlerBase : public server::handlers::HttpHandlerBase {
 public:
  HandlerBase(const components::ComponentConfig& config, const components::ComponentContext& context);

 protected:
  std::string Execute(
      const server::http::HttpRequest& request,
      const std::string& rate_limit_key,
      const std::function<formats::json::Value()>& action
  ) const;
  void ApplyRateLimitHeaders(
      const server::http::HttpRequest& request,
      const RateLimiter::Result& result
  ) const;

  RateLimiter::Result CheckRateAndApplyHeaders(
      const server::http::HttpRequest& request,
      const std::string& rate_limit_key
  ) const;

  RecipeStorage& storage_;
  RateLimiter& rate_limiter_;
  CacheManager& cache_manager_;
};

class RootHandler final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-root";

  using HttpHandlerBase::HttpHandlerBase;

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&
  ) const override;
};

class CreateUserHandler final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-users-create";

  CreateUserHandler(const components::ComponentConfig& config, const components::ComponentContext& context);

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request,
      const formats::json::Value& request_json,
      server::request::RequestContext&
  ) const override;

 private:
  RecipeStorage& storage_;
  RateLimiter& rate_limiter_;
  CacheManager& cache_manager_;
};

class FindUserByLoginHandler final : public HandlerBase {
 public:
  static constexpr std::string_view kName = "handler-users-by-login";

  using HandlerBase::HandlerBase;

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&
  ) const override;
};

class SearchUsersByNameHandler final : public HandlerBase {
 public:
  static constexpr std::string_view kName = "handler-users-search";

  using HandlerBase::HandlerBase;

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&
  ) const override;
};

class CreateRecipeHandler final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-recipes-create";

  CreateRecipeHandler(const components::ComponentConfig& config, const components::ComponentContext& context);

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request,
      const formats::json::Value& request_json,
      server::request::RequestContext&
  ) const override;

 private:
  RecipeStorage& storage_;
  RateLimiter& rate_limiter_;
  CacheManager& cache_manager_;
};

class GetRecipesHandler final : public HandlerBase {
 public:
  static constexpr std::string_view kName = "handler-recipes-list";

  using HandlerBase::HandlerBase;

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&
  ) const override;
};

class SearchRecipesHandler final : public HandlerBase {
 public:
  static constexpr std::string_view kName = "handler-recipes-search";

  using HandlerBase::HandlerBase;

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&
  ) const override;
};

class AddIngredientHandler final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-recipes-add-ingredient";

  AddIngredientHandler(const components::ComponentConfig& config, const components::ComponentContext& context);

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request,
      const formats::json::Value& request_json,
      server::request::RequestContext&
  ) const override;

 private:
  RecipeStorage& storage_;
  RateLimiter& rate_limiter_;
  CacheManager& cache_manager_;
};

class GetIngredientsHandler final : public HandlerBase {
 public:
  static constexpr std::string_view kName = "handler-recipes-ingredients";

  using HandlerBase::HandlerBase;

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&
  ) const override;
};

class GetUserRecipesHandler final : public HandlerBase {
 public:
  static constexpr std::string_view kName = "handler-user-recipes";

  using HandlerBase::HandlerBase;

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&
  ) const override;
};

class AddFavoriteHandler final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-user-favorites-add";

  AddFavoriteHandler(const components::ComponentConfig& config, const components::ComponentContext& context);

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request,
      const formats::json::Value& request_json,
      server::request::RequestContext&
  ) const override;

 private:
  RecipeStorage& storage_;
  RateLimiter& rate_limiter_;
  CacheManager& cache_manager_;
};

class GetFavoritesHandler final : public HandlerBase {
 public:
  static constexpr std::string_view kName = "handler-user-favorites-list";

  using HandlerBase::HandlerBase;

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&
  ) const override;
};

components::ComponentList MakeRecipeComponentList();

}  // namespace recipe_manager

USERVER_NAMESPACE_END
