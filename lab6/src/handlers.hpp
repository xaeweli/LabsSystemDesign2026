#pragma once

#include <string>
#include <string_view>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>

#include "event_producer.hpp"
#include "recipe_read_model.hpp"
#include "recipe_storage.hpp"

USERVER_NAMESPACE_BEGIN

namespace recipe_manager {

class BaseHandler : public server::handlers::HttpHandlerBase {
 public:
  BaseHandler(const components::ComponentConfig& config, const components::ComponentContext& context);

 protected:
  template <typename F>
  std::string Execute(const server::http::HttpRequest& request, F&& action) const {
    try {
      return formats::json::ToString(action());
    } catch (const ApiException& ex) {
      request.GetHttpResponse().SetStatus(ex.GetStatus());
      return formats::json::ToString(ErrorJson(ex.what()));
    } catch (const std::exception& ex) {
      request.GetHttpResponse().SetStatus(server::http::HttpStatus::kBadRequest);
      return formats::json::ToString(ErrorJson(ex.what()));
    }
  }

  RecipeStorage& storage_;
  EventProducer& producer_;
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
  EventProducer& producer_;
};

class FindUserByLoginHandler final : public BaseHandler {
 public:
  static constexpr std::string_view kName = "handler-users-by-login";

  using BaseHandler::BaseHandler;

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&
  ) const override;
};

class SearchUsersByNameHandler final : public BaseHandler {
 public:
  static constexpr std::string_view kName = "handler-users-search";

  using BaseHandler::BaseHandler;

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
  EventProducer& producer_;
};

class GetRecipesHandler final : public BaseHandler {
 public:
  static constexpr std::string_view kName = "handler-recipes-list";

  GetRecipesHandler(const components::ComponentConfig& config, const components::ComponentContext& context);

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&
  ) const override;

 private:
  RecipeReadModel& read_model_;
};

class SearchRecipesHandler final : public BaseHandler {
 public:
  static constexpr std::string_view kName = "handler-recipes-search";

  SearchRecipesHandler(const components::ComponentConfig& config, const components::ComponentContext& context);

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&
  ) const override;

 private:
  RecipeReadModel& read_model_;
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
  EventProducer& producer_;
};

class GetIngredientsHandler final : public BaseHandler {
 public:
  static constexpr std::string_view kName = "handler-recipes-ingredients";

  using BaseHandler::BaseHandler;

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&
  ) const override;
};

class GetUserRecipesHandler final : public BaseHandler {
 public:
  static constexpr std::string_view kName = "handler-user-recipes";

  GetUserRecipesHandler(const components::ComponentConfig& config, const components::ComponentContext& context);

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&
  ) const override;

 private:
  RecipeReadModel& read_model_;
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
  EventProducer& producer_;
};

class GetFavoritesHandler final : public BaseHandler {
 public:
  static constexpr std::string_view kName = "handler-user-favorites-list";

  GetFavoritesHandler(const components::ComponentConfig& config, const components::ComponentContext& context);

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&
  ) const override;

 private:
  RecipeReadModel& read_model_;
};

components::ComponentList MakeRecipeComponentList();

}  // namespace recipe_manager

USERVER_NAMESPACE_END
