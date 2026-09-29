# The web emulator: build the firmware to WebAssembly, serve it with nginx.
# Built by Coolify from the repo root (dockerfile_location /emu/Dockerfile).
FROM emscripten/emsdk:4.0.10 AS build
WORKDIR /src
COPY . .
RUN sh emu/build.sh

FROM nginx:1.27-alpine
COPY emu/nginx.conf /etc/nginx/conf.d/default.conf
COPY --from=build /src/emu/dist /usr/share/nginx/html
