FROM ghcr.io/userver-framework/ubuntu-22.04-userver:latest AS build

WORKDIR /app

RUN apt-get update && apt-get install -y --no-install-recommends \
    librabbitmq-dev \
    && rm -rf /var/lib/apt/lists/*

COPY . .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build -j

FROM ghcr.io/userver-framework/ubuntu-22.04-userver:latest

RUN apt-get update && apt-get install -y --no-install-recommends \
    librabbitmq4 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /app/build/recipe-manager-event-driven /app/recipe-manager-event-driven
COPY configs /app/configs

EXPOSE 8080

CMD ["/app/recipe-manager-event-driven", "--config", "/app/configs/static_config.yaml", "--config_vars", "/app/configs/config_vars.yaml"]
