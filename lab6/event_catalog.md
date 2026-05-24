# Event Catalog

## Variant 23 — Recipe Management System (Allrecipes)

---

## Overview

| # | Event Name | Version | Status | Schema |
|---|------------|---------|--------|--------|
| 1 | `UserCreated` | 1.0 | Active | `event/user_created.v1.json` |
| 2 | `RecipeCreated` | 1.0 | Active | `event/recipe_created.v1.json` |
| 3 | `IngredientAdded` | 1.0 | Active | `event/ingredient_added.v1.json` |
| 4 | `RecipeFavorited` | 1.0 | Active | `event/recipe_favorited.v1.json` |

---

## 1. UserCreated

Fired when a new user registers in the system.

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `event_type` | string | yes | `"UserCreated"` |
| `event_id` | string (uuid) | yes | Unique event identifier |
| `timestamp` | string (ISO 8601) | yes | When the event occurred |
| `payload.user_id` | string (uuid) | yes | New user's ID |
| `payload.username` | string | yes | User's login name |
| `payload.email` | string | yes | User's email address |

**Producer:** `User Service` (`POST /users`)  
**Consumers:** `Read Model Updater`, `Analytics Logger`

---

## 2. RecipeCreated

Fired when a user creates a new recipe.

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `event_type` | string | yes | `"RecipeCreated"` |
| `event_id` | string (uuid) | yes | Unique event identifier |
| `timestamp` | string (ISO 8601) | yes | When the event occurred |
| `payload.recipe_id` | string (uuid) | yes | New recipe's ID |
| `payload.user_id` | string (uuid) | yes | Author's user ID |
| `payload.title` | string | yes | Recipe title |
| `payload.cooking_time_minutes` | integer | yes | Cooking time in minutes |

**Producer:** `Recipe Service` (`POST /recipes`)  
**Consumers:** `Read Model Updater`, `Search Index Updater`, `Analytics Logger`

---

## 3. IngredientAdded

Fired when an ingredient is added to an existing recipe.

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `event_type` | string | yes | `"IngredientAdded"` |
| `event_id` | string (uuid) | yes | Unique event identifier |
| `timestamp` | string (ISO 8601) | yes | When the event occurred |
| `payload.recipe_id` | string (uuid) | yes | Recipe's ID |
| `payload.ingredient_id` | string (uuid) | yes | New ingredient's ID |
| `payload.name` | string | yes | Ingredient name |
| `payload.amount` | string | yes | Quantity |
| `payload.unit` | string | yes | Unit of measurement |

**Producer:** `Recipe Service` (`POST /recipes/{id}/ingredients`)  
**Consumers:** `Read Model Updater`, `Search Index Updater`

---

## 4. RecipeFavorited

Fired when a user adds a recipe to their favorites.

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `event_type` | string | yes | `"RecipeFavorited"` |
| `event_id` | string (uuid) | yes | Unique event identifier |
| `timestamp` | string (ISO 8601) | yes | When the event occurred |
| `payload.user_id` | string (uuid) | yes | User who favorited |
| `payload.recipe_id` | string (uuid) | yes | Recipe that was favorited |

**Producer:** `Favorites Service` (`POST /users/{id}/favorites`)  
**Consumers:** `Read Model Updater`, `Analytics Logger`

---

## Event Flow Diagram

```
                          ┌──────────────────────┐
                          │   recipe.events (topic)│
                          └──────────┬───────────┘
                                     │
              ┌──────────────────────┼──────────────────────┐
              │                      │                      │
              ▼                      ▼                      ▼
    ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
    │ recipe.read-    │  │ recipe.search-  │  │ recipe.analytics│
    │ model           │  │ index           │  │                 │
    └────────┬────────┘  └────────┬────────┘  └────────┬────────┘
             │                    │                     │
             ▼                    ▼                     ▼
    ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
    │ ReadModel       │  │ SearchIndex     │  │ AnalyticsLogger │
    │ Updater         │  │ Updater         │  │                 │
    └─────────────────┘  └─────────────────┘  └─────────────────┘
```

## Delivery Guarantees by Event

| Event | Delivery | Queue Type | Ack Mode | Retry |
|-------|----------|------------|----------|-------|
| `UserCreated` | At-least-once | Durable | Manual | 3 attempts |
| `RecipeCreated` | At-least-once | Durable | Manual | 3 attempts |
| `IngredientAdded` | At-least-once | Durable | Manual | 3 attempts |
| `RecipeFavorited` | At-least-once | Durable | Manual | 3 attempts |
