#pragma once

#include <foxhttp/foxhttp.hpp>

namespace foxhttp::examples {

/// Same behavior as ``CorsMiddleware`` but with ``MiddlewarePriority::Lowest`` so it runs before
/// other stock middleware (logger ``high``, ``response_time`` ``low``) when the chain auto-sorts.
class first_cors_middleware : public middleware::CorsMiddleware {
 public:
  using middleware::CorsMiddleware::CorsMiddleware;
  middleware::MiddlewarePriority priority() const override {
    return middleware::MiddlewarePriority::Lowest;
  }
};

}  // namespace foxhttp::examples
