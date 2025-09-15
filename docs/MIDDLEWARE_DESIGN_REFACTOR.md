# middleware Design Refactor

## Overview

This document explains the refactoring of the middleware system to remove the redundant `MiddlewareBuilder` class and establish a cleaner, more maintainable architecture.

## Problem Analysis

### Issues with MiddlewareBuilder

1. **Redundancy**: `MiddlewareBuilder` was essentially a wrapper around `FunctionalMiddleware`
2. **Design Inconsistency**: Two different ways to create middlewares (builder pattern vs. direct classes)
3. **Maintenance Burden**: Duplicate APIs requiring separate maintenance
4. **Type Safety**: Less type-safe compared to dedicated middleware classes
5. **Testing**: Harder to unit test compared to dedicated classes

### Root Cause

The `MiddlewareBuilder` was created as a convenience tool, but it became redundant when we established the pattern of having dedicated middleware classes in `middleware/basic/` and `middleware/advanced/` folders.

## Solution

### 1. Removed MiddlewareBuilder

- Eliminated the `MiddlewareBuilder` class entirely
- Replaced with `SimpleFunctionalMiddleware` for quick prototyping only
- Marked as deprecated for future removal

### 2. Created Dedicated middleware Classes

#### Basic Middlewares (`middleware/basic/`)

- **LoggerMiddleware**: Proper logging with timing information
- **CorsMiddleware**: CORS handling with configurable options
- **ResponseTimeMiddleware**: Response time measurement

#### Advanced Middlewares (`middleware/advanced/`)

- **AuthMiddleware**: Authentication handling
- **CompressionMiddleware**: Response compression
- **RateLimitMiddleware**: Rate limiting

### 3. Updated Utility Functions

#### Legacy Functions (for backward compatibility)

```cpp
// Still available but deprecated
middleware_utils::create_logger("LoggerName");
middleware_utils::create_cors("*");
middleware_utils::create_response_time();
```

#### New Factory Functions (recommended)

```cpp
// Recommended approach
middleware_utils::factories::create_logger_middleware("LoggerName");
middleware_utils::factories::create_cors_middleware("*");
middleware_utils::factories::create_response_time_middleware();
```

## Usage Patterns

### 1. Recommended: Dedicated middleware Classes

```cpp
// Create middleware chain
middleware_chain chain(io_context);

// Use specific middleware classes
chain.use(middleware_utils::factories::create_logger_middleware("RequestLogger"));
chain.use(middleware_utils::factories::create_cors_middleware("*"));
chain.use(std::make_shared<CustomAuthMiddleware>("secret-key"));
chain.use(middleware_utils::factories::create_response_time_middleware());
```

### 2. Custom middleware Implementation

```cpp
class CustomMiddleware : public middleware {
public:
    std::string name() const override { return "CustomMiddleware"; }
    middleware_priority priority() const override { return middleware_priority::normal; }

    void operator()(request_context &ctx, http::response<http::string_body> &res,
                    std::function<void()> next) override {
        // Synchronous processing
        next();
    }

    void operator()(request_context &ctx, http::response<http::string_body> &res,
                    std::function<void()> next, AsyncMiddlewareCallback callback) override {
        // Asynchronous processing
        callback(middleware_result::continue_, "");
    }
};
```

### 3. Legacy: Simple Functional middleware (Deprecated)

```cpp
// Only for quick prototyping - will be removed in future versions
auto quick_mw = std::make_shared<SimpleFunctionalMiddleware>("QuickMW",
    [](const request_context& ctx, auto& res, auto next) {
        // Processing logic
        next();
    });
```

## Benefits of the Refactor

### 1. **Better Type Safety**

- Dedicated classes provide compile-time type checking
- Clear interfaces and contracts
- Better IDE support and autocompletion

### 2. **Improved Maintainability**

- Single responsibility principle
- Easier to test individual middleware classes
- Clear separation of concerns

### 3. **Enhanced Performance**

- No runtime overhead from builder pattern
- Direct instantiation of middleware objects
- Better memory layout and cache efficiency

### 4. **Better Documentation**

- Each middleware class can have detailed documentation
- Clear examples and usage patterns
- Self-documenting code structure

### 5. **Easier Testing**

- Unit tests for individual middleware classes
- Mock objects for testing middleware chains
- Better test coverage and reliability

## Migration Guide

### From MiddlewareBuilder to Dedicated Classes

#### Before (Deprecated)

```cpp
auto middleware = MiddlewareBuilder()
    .name("MyMiddleware")
    .priority(middleware_priority::high)
    .sync([](const request_context& ctx, auto& res, auto next) {
        // Processing logic
        next();
    })
    .build();
```

#### After (Recommended)

```cpp
class MyMiddleware : public middleware {
public:
    std::string name() const override { return "MyMiddleware"; }
    middleware_priority priority() const override { return middleware_priority::high; }

    void operator()(request_context &ctx, http::response<http::string_body> &res,
                    std::function<void()> next) override {
        // Processing logic
        next();
    }
};

// Usage
chain.use(std::make_shared<MyMiddleware>());
```

### From Utility Functions to Factory Functions

#### Before (Legacy)

```cpp
chain.use(middleware_utils::create_logger("LoggerName"));
```

#### After (Recommended)

```cpp
chain.use(middleware_utils::factories::create_logger_middleware("LoggerName"));
```

## Future Plans

### Phase 1: Deprecation (Current)

- Mark `MiddlewareBuilder` as deprecated
- Provide migration examples
- Update documentation

### Phase 2: Removal (Next Version)

- Remove `MiddlewareBuilder` entirely
- Remove legacy utility functions
- Keep only factory functions and dedicated classes

### Phase 3: Enhancement

- Add more middleware classes
- Improve async support
- Add middleware composition features

## Best Practices

### 1. **Use Dedicated Classes**

- Always prefer dedicated middleware classes over functional approaches
- Implement both sync and async versions when needed
- Use appropriate priority levels

### 2. **Follow Naming Conventions**

- Use descriptive names ending with "middleware"
- Use consistent naming patterns
- Document the purpose and behavior

### 3. **Handle Errors Properly**

- Use appropriate `middleware_result` values
- Provide meaningful error messages
- Handle timeouts gracefully

### 4. **Optimize Performance**

- Use appropriate priority levels
- Implement conditional execution when possible
- Monitor performance with statistics

### 5. **Write Tests**

- Unit tests for individual middleware classes
- Integration tests for middleware chains
- Performance tests for critical paths

## Conclusion

The refactoring removes redundancy, improves maintainability, and establishes a cleaner architecture. While it requires some migration effort, the benefits in terms of type safety, performance, and maintainability make it worthwhile.

The new design follows established software engineering principles and provides a solid foundation for future enhancements to the FoxHTTP middleware system.
