# Changelog

## Unreleased (Coroutine branch)

### Changed

- **Language**: C++ **20** required (was C++17).
- **Server I/O**: `server` / `ssl_server` accept loops, `session` / `ssl_session` HTTP+TLS handshake+read/write, and `ws_session` / `wss_session` WebSocket paths use **Boost.Asio coroutines** (`co_spawn`, `awaitable`, `use_awaitable`).
- **Middleware**: Public middleware API unchanged; sessions await `middleware_chain::execute_async` via an internal bridge (`foxhttp::detail::await_middleware_async.hpp`) that posts completion so sync callbacks are safe.
- **Read timeouts**: Header/body timeouts cancel the pending read; the session coroutine sends 408 when the abort is due to timeout.

### Porting

- Bump your consumer project to **C++20** if you link `foxhttp::foxhttplib` or include headers that use `std::atomic` on enums / Asio awaitable types in server interfaces.
