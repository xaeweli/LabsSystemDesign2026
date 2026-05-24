#include "handlers.hpp"

#include <optional>
#include <string>

#include <userver/formats/json.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/content_type.hpp>
#include <userver/server/http/http_status.hpp>

#include "api_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace recipe_manager {

namespace {

formats::json::Value RecipesToJson(const MongoStorage& storage, const std::vector<Recipe>& recipes) {
  formats::json::ValueBuilder builder(formats::common::Type::kArray);
  for (const auto& recipe : recipes) {
    builder.PushBack(storage.RecipeToJson(recipe));
  }
  return builder.ExtractValue();
}

}  // namespace

HandlerBase::HandlerBase(const components::ComponentConfig& config, const components::ComponentContext& context)
    : HttpHandlerBase(config, context), storage_(context.FindComponent<MongoStorage>()) {}

std::string RootHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&
) const {
  formats::json::ValueBuilder endpoints(formats::common::Type::kArray);
  endpoints.PushBack("POST /users");
  endpoints.PushBack("GET /users/login/{username}");
  endpoints.PushBack("GET /users/search?first_name=Iv*&last_name=Pet*");
  endpoints.PushBack("POST /recipes");
  endpoints.PushBack("GET /recipes");
  endpoints.PushBack("GET /recipes/search?title=*pie*");
  endpoints.PushBack("POST /recipes/{recipe_id}/ingredients");
  endpoints.PushBack("GET /recipes/{recipe_id}/ingredients");
  endpoints.PushBack("GET /users/{user_id}/recipes");
  endpoints.PushBack("POST /users/{user_id}/favorites");
  endpoints.PushBack("GET /users/{user_id}/favorites");

  formats::json::ValueBuilder builder(formats::common::Type::kObject);
  builder["service"] = "recipe-manager-userver-mongodb";
  builder["status"] = "ok";
  builder["variant"] = "23";
  builder["description"] = "Recipe management API (MongoDB-backed)";
  builder["endpoints"] = endpoints.ExtractValue();

  request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);
  return formats::json::ToString(builder.ExtractValue());
}

std::string HandlerBase::Execute(
    const server::http::HttpRequest& request,
    const std::function<formats::json::Value()>& action
) const {
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

CreateUserHandler::CreateUserHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
    : HttpHandlerJsonBase(config, context), storage_(context.FindComponent<MongoStorage>()) {}

formats::json::Value CreateUserHandler::HandleRequestJsonThrow(
    const server::http::HttpRequest& request,
    const formats::json::Value& request_json,
    server::request::RequestContext&
) const {
  try {
    const auto user = storage_.CreateUser(
        RequireField<std::string>(request_json, "username"),
        RequireField<std::string>(request_json, "first_name"),
        RequireField<std::string>(request_json, "last_name"),
        RequireField<std::string>(request_json, "email"),
        RequireField<std::string>(request_json, "password")
    );
    request.GetHttpResponse().SetStatus(server::http::HttpStatus::kCreated);
    return storage_.UserToJson(user);
  } catch (const ApiException& ex) {
    request.GetHttpResponse().SetStatus(ex.GetStatus());
    return ErrorJson(ex.what());
  }
}

std::string FindUserByLoginHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&
) const {
  return Execute(request, [&] {
    const auto user = storage_.FindUserByUsername(request.GetPathArg("username"));
    if (!user) {
      throw ApiException(server::http::HttpStatus::kNotFound, "User not found");
    }
    return storage_.UserToJson(*user);
  });
}

std::string SearchUsersByNameHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&
) const {
  return Execute(request, [&] {
    formats::json::ValueBuilder builder(formats::common::Type::kArray);
    const auto users = storage_.SearchUsersByNameMask(
        request.HasArg("first_name") ? std::make_optional(request.GetArg("first_name")) : std::nullopt,
        request.HasArg("last_name") ? std::make_optional(request.GetArg("last_name")) : std::nullopt
    );
    for (const auto& user : users) {
      builder.PushBack(storage_.UserToJson(user));
    }
    return builder.ExtractValue();
  });
}

CreateRecipeHandler::CreateRecipeHandler(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
) : HttpHandlerJsonBase(config, context), storage_(context.FindComponent<MongoStorage>()) {}

formats::json::Value CreateRecipeHandler::HandleRequestJsonThrow(
    const server::http::HttpRequest& request,
    const formats::json::Value& request_json,
    server::request::RequestContext&
) const {
  try {
    const auto recipe = storage_.CreateRecipe(
        RequireField<std::string>(request_json, "user_id"),
        RequireField<std::string>(request_json, "title"),
        OptionalString(request_json, "description").value_or(""),
        RequireField<std::string>(request_json, "instructions"),
        RequireField<int>(request_json, "cooking_time_minutes")
    );
    request.GetHttpResponse().SetStatus(server::http::HttpStatus::kCreated);
    return storage_.RecipeToJson(recipe);
  } catch (const ApiException& ex) {
    request.GetHttpResponse().SetStatus(ex.GetStatus());
    return ErrorJson(ex.what());
  }
}

std::string GetRecipesHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&
) const {
  return Execute(request, [&] {
    return RecipesToJson(storage_, storage_.GetRecipes());
  });
}

std::string SearchRecipesHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&
) const {
  return Execute(request, [&] {
    if (!request.HasArg("title")) {
      throw ApiException(server::http::HttpStatus::kBadRequest, "title is required");
    }
    return RecipesToJson(storage_, storage_.SearchRecipesByTitle(request.GetArg("title")));
  });
}

AddIngredientHandler::AddIngredientHandler(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
) : HttpHandlerJsonBase(config, context), storage_(context.FindComponent<MongoStorage>()) {}

formats::json::Value AddIngredientHandler::HandleRequestJsonThrow(
    const server::http::HttpRequest& request,
    const formats::json::Value& request_json,
    server::request::RequestContext&
) const {
  try {
    const auto ingredient = storage_.AddIngredientToRecipe(
        request.GetPathArg("recipe_id"),
        RequireField<std::string>(request_json, "name"),
        RequireField<std::string>(request_json, "amount"),
        OptionalString(request_json, "unit").value_or("")
    );
    request.GetHttpResponse().SetStatus(server::http::HttpStatus::kCreated);
    return storage_.IngredientToJson(ingredient);
  } catch (const ApiException& ex) {
    request.GetHttpResponse().SetStatus(ex.GetStatus());
    return ErrorJson(ex.what());
  }
}

std::string GetIngredientsHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&
) const {
  return Execute(request, [&] {
    formats::json::ValueBuilder builder(formats::common::Type::kArray);
    for (const auto& ingredient : storage_.GetRecipeIngredients(request.GetPathArg("recipe_id"))) {
      builder.PushBack(storage_.IngredientToJson(ingredient));
    }
    return builder.ExtractValue();
  });
}

std::string GetUserRecipesHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&
) const {
  return Execute(request, [&] {
    return RecipesToJson(storage_, storage_.GetUserRecipes(request.GetPathArg("user_id")));
  });
}

AddFavoriteHandler::AddFavoriteHandler(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
) : HttpHandlerJsonBase(config, context), storage_(context.FindComponent<MongoStorage>()) {}

formats::json::Value AddFavoriteHandler::HandleRequestJsonThrow(
    const server::http::HttpRequest& request,
    const formats::json::Value& request_json,
    server::request::RequestContext&
) const {
  try {
    const auto recipe = storage_.AddFavoriteRecipe(
        request.GetPathArg("user_id"),
        RequireField<std::string>(request_json, "recipe_id")
    );
    return storage_.RecipeToJson(recipe);
  } catch (const ApiException& ex) {
    request.GetHttpResponse().SetStatus(ex.GetStatus());
    return ErrorJson(ex.what());
  }
}

std::string GetFavoritesHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&
) const {
  return Execute(request, [&] {
    return RecipesToJson(storage_, storage_.GetFavoriteRecipes(request.GetPathArg("user_id")));
  });
}

components::ComponentList MakeRecipeComponentList() {
  return components::MinimalServerComponentList()
      .Append<components::Mongo>("mongo")
      .Append<MongoStorage>()
      .Append<RootHandler>()
      .Append<CreateUserHandler>()
      .Append<FindUserByLoginHandler>()
      .Append<SearchUsersByNameHandler>()
      .Append<CreateRecipeHandler>()
      .Append<GetRecipesHandler>()
      .Append<SearchRecipesHandler>()
      .Append<AddIngredientHandler>()
      .Append<GetIngredientsHandler>()
      .Append<GetUserRecipesHandler>()
      .Append<AddFavoriteHandler>()
      .Append<GetFavoritesHandler>();
}

}  // namespace recipe_manager

USERVER_NAMESPACE_END
