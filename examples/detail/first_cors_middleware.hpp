#pragma once

#include <foxhttp/middleware/basic/cors_middleware.hpp>

namespace foxhttp::examples {

/// Same behavior as ``cors_middleware`` but with ``middleware_priority::lowest`` so it runs before
/// other stock middleware (logger ``high``, ``response_time`` ``low``) when the chain auto-sorts.
class first_cors_middleware : public cors_middleware {
 public:
  using cors_middleware::cors_middleware;
  middleware_priority priority() const override { return middleware_priority::lowest; }
};

}  // namespace foxhttp::examples
