# Enhanced middleware System

The FoxHTTP middleware system has been significantly improved with new features and capabilities.

## Key Improvements

### 1. Asynchronous Support

- **True async execution**: Middlewares can now execute asynchronously using Boost.Asio
- **Non-blocking operations**: Long-running operations don't block the request processing
- **Callback-based**: Uses callback functions for async completion handling

```cpp
// Async middleware example
class AsyncMiddleware : public middleware {
public:
    void operator()(request_context &ctx, http::response<http::string_body> &res,
                    std::function<void()> next, AsyncMiddlewareCallback callback) override {
        // Perform async operation
        std::thread([next, callback]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            next();
            callback(middleware_result::continue_);
        }).detach();
    }
};
```

### 2. Priority System

- **Automatic sorting**: Middlewares are automatically sorted by priority
- **Flexible priorities**: 5 priority levels from lowest to highest
- **Custom priorities**: Support for custom priority values

```cpp
// Priority middleware example
class HighPriorityMiddleware : public PriorityMiddleware<middleware_priority::high> {
    // This middleware will execute before normal priority middlewares
};
```

### 3. Conditional Execution

- **Smart filtering**: Middlewares can decide whether to execute based on request context
- **Performance optimization**: Skip unnecessary processing
- **Flexible conditions**: Use any condition function

```cpp
// Conditional middleware example
auto conditional_mw = std::make_shared<ConditionalMiddleware>(
    std::make_shared<SomeMiddleware>(),
    [](const request_context& ctx) {
        return ctx.method() == http::verb::post; // Only for POST requests
    }
);
```

### 4. Statistics and Monitoring

- **Execution tracking**: Count executions, errors, and timeouts
- **Performance metrics**: Track execution time for each middleware
- **Real-time monitoring**: Get statistics during runtime

```cpp
// Enable statistics
chain.enable_statistics(true);

// Get statistics
auto stats = chain.get_statistics();
chain.print_statistics();
```

### 5. Enhanced Error Handling

- **Granular error handling**: Different error types (error, timeout, stop)
- **Recovery mechanisms**: Better error recovery and reporting
- **Custom error handlers**: Configurable error and timeout handlers

```cpp
// Custom error handler
chain.set_error_handler([](request_context &ctx,
                          http::response<http::string_body> &res,
                          const std::exception &e) {
    // Custom error handling logic
});
```

### 6. Timeout Management

- **Global timeouts**: Set timeout for entire middleware chain
- **Per-middleware timeouts**: Individual timeout for each middleware
- **Automatic cancellation**: Timers are automatically managed

```cpp
// Set timeouts
chain.set_global_timeout(std::chrono::milliseconds(5000));
auto mw = std::make_shared<TimeoutMiddleware>();
mw->set_timeout(std::chrono::milliseconds(1000));
```

### 7. Lifecycle Management

- **Dynamic management**: Add, remove, and reorder middlewares at runtime
- **Priority-based insertion**: Insert middlewares by priority
- **State management**: Pause, resume, and cancel execution

```cpp
// Dynamic middleware management
chain.insert_by_priority(std::make_shared<NewMiddleware>());
chain.remove("OldMiddleware");
chain.pause();
chain.resume();
chain.cancel();
```

## Usage Examples

### Basic Usage with Builder Pattern

```cpp
// Create middleware using builder pattern
auto middleware = MiddlewareBuilder()
    .name("MyMiddleware")
    .priority(middleware_priority::high)
    .timeout(std::chrono::milliseconds(1000))
    .condition([](const request_context& ctx) {
        return ctx.path().starts_with("/api");
    })
    .sync([](const request_context& ctx, http::response<http::string_body>& res, std::function<void()> next) {
        // Synchronous processing
        next();
    })
    .async([](const request_context& ctx, http::response<http::string_body>& res,
              std::function<void()> next, AsyncMiddlewareCallback callback) {
        // Asynchronous processing
        callback(middleware_result::continue_);
    })
    .build();

chain.use(middleware);
```

### Using Utility Functions

```cpp
// Create common middlewares easily
chain.use(middleware_utils::create_logger("RequestLogger"));
chain.use(middleware_utils::create_cors("*"));
chain.use(middleware_utils::create_response_time());
chain.use(middleware_utils::create_rate_limit(100));
```

### Async Execution

```cpp
// Execute middleware chain asynchronously
chain.execute_async(ctx, res, [](middleware_result result, const std::string& error) {
    if (result == middleware_result::continue_) {
        std::cout << "Request processed successfully" << std::endl;
    } else {
        std::cout << "Error: " << error << std::endl;
    }
});
```

## Performance Benefits

1. **Non-blocking execution**: Async middlewares don't block the event loop
2. **Priority-based ordering**: Critical middlewares execute first
3. **Conditional execution**: Skip unnecessary processing
4. **Timeout protection**: Prevent hanging requests
5. **Statistics tracking**: Monitor and optimize performance

## Migration Guide

### From Old System

```cpp
// Old way
class OldMiddleware : public middleware {
public:
    void operator()(request_context &ctx, http::response<http::string_body> &res,
                    std::function<void()> next) override {
        // Processing logic
        next();
    }
};

// New way (backward compatible)
class NewMiddleware : public middleware {
public:
    std::string name() const override { return "NewMiddleware"; }
    middleware_priority priority() const override { return middleware_priority::normal; }

    void operator()(request_context &ctx, http::response<http::string_body> &res,
                    std::function<void()> next) override {
        // Same processing logic
        next();
    }

    // Optional: Add async support
    void operator()(request_context &ctx, http::response<http::string_body> &res,
                    std::function<void()> next, AsyncMiddlewareCallback callback) override {
        // Async processing or fallback to sync
        middleware::operator()(ctx, res, next, callback);
    }
};
```

## Best Practices

1. **Use appropriate priorities**: Set priorities based on middleware importance
2. **Implement timeouts**: Always set reasonable timeouts for async operations
3. **Handle errors gracefully**: Implement proper error handling in async callbacks
4. **Use statistics**: Monitor middleware performance and optimize accordingly
5. **Conditional execution**: Use conditions to skip unnecessary processing
6. **Resource management**: Properly manage timers and async operations

## API Reference

### middleware Class

- `name()`: Get middleware name
- `priority()`: Get middleware priority
- `should_execute()`: Check if middleware should execute
- `timeout()`: Get middleware timeout
- `stats()`: Get execution statistics

### middleware_chain Class

- `use()`: Add middleware to chain
- `insert_by_priority()`: Insert middleware by priority
- `remove()`: Remove middleware by name
- `execute_async()`: Execute chain asynchronously
- `set_global_timeout()`: Set global timeout
- `enable_statistics()`: Enable/disable statistics
- `get_statistics()`: Get execution statistics
- `pause()`/`resume()`: Control execution
- `cancel()`: Cancel execution

### Utility Functions

- `create_logger()`: Create logging middleware
- `create_cors()`: Create CORS middleware
- `create_response_time()`: Create response time middleware
- `create_rate_limit()`: Create rate limiting middleware
- `create_basic_auth()`: Create basic authentication middleware
