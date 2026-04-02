#include <foxhttp/router/route.hpp>
#include <foxhttp/server/request_context.hpp>
#include <regex>
#include <stdexcept>

namespace foxhttp::router {

/* ------------------------- StaticRoute ------------------------- */
StaticRoute::StaticRoute(std::string pattern, std::shared_ptr<ApiHandler> handler)
    : pattern_(std::move(pattern)), handler_(std::move(handler)) {}

ApiHandler *StaticRoute::match(const std::string &path, server::RequestContext &ctx) const {
  (void)ctx;
  return (path == pattern_) ? handler_.get() : nullptr;
}

std::string StaticRoute::pattern() const { return pattern_; }

std::shared_ptr<ApiHandler> StaticRoute::handler() const { return handler_; }

/* ------------------------- DynamicRoute ------------------------- */
DynamicRoute::DynamicRoute(std::string pattern, std::shared_ptr<ApiHandler> handler, std::regex regex_pattern,
                             std::vector<std::string> param_names)
    : pattern_(std::move(pattern)),
      handler_(std::move(handler)),
      regex_pattern_(std::move(regex_pattern)),
      param_names_(std::move(param_names)) {}

ApiHandler *DynamicRoute::match(const std::string &path, server::RequestContext &ctx) const {
  std::smatch matches;
  if (std::regex_match(path, matches, regex_pattern_)) {
    extract_path_params(matches, ctx);
    return handler_.get();
  }
  return nullptr;
}

std::string DynamicRoute::pattern() const { return pattern_; }

const std::regex &DynamicRoute::regex() const { return regex_pattern_; }

const std::vector<std::string> &DynamicRoute::param_names() const { return param_names_; }

std::shared_ptr<ApiHandler> DynamicRoute::handler() const { return handler_; }

void DynamicRoute::extract_path_params(const std::smatch &matches, server::RequestContext &ctx) const {
  std::unordered_map<std::string, std::string> params;
  for (size_t i = 1; i < matches.size() && i - 1 < param_names_.size(); ++i) {
    if (matches[i].matched) {
      params[param_names_[i - 1]] = matches[i].str();
    }
  }
  if (!params.empty()) {
    ctx.set_path_parameters(params);
  }
}

/* ------------------------- route_builder ------------------------- */

std::pair<std::regex, std::vector<std::string>> RouteBuilder::parse_pattern(const std::string &pattern) {
  std::vector<std::string> param_names;
  std::string regex_pattern;
  regex_pattern.reserve(pattern.size() * 2);

  std::size_t start = 0;
  while (true) {
    std::size_t pos = pattern.find('{', start);
    if (pos == std::string::npos) {
      regex_pattern += regex_escape(pattern.substr(start));
      break;
    }
    // add static part
    regex_pattern += regex_escape(pattern.substr(start, pos - start));
    std::size_t end = pattern.find('}', pos);
    if (end == std::string::npos) {
      throw std::invalid_argument("Unclosed parameter in pattern: " + pattern);
    }
    std::string param_name = pattern.substr(pos + 1, end - pos - 1);
    if (param_name.empty()) {
      throw std::invalid_argument("Empty parameter name in pattern: " + pattern);
    }
    param_names.push_back(param_name);
    regex_pattern += "([^/]+)";
    start = end + 1;
  }

  regex_pattern = "^" + regex_pattern + "$";
  return {std::regex(regex_pattern), param_names};
}

std::string RouteBuilder::regex_escape(const std::string &str) {
  // simple character-wise escape of regex special chars
  static const std::string special = R"(.^$|()[]{}*+?\)";
  std::string out;
  out.reserve(str.size() * 2);
  for (char c : str) {
    if (special.find(c) != std::string::npos) {
      out.push_back('\\');
    }
    out.push_back(c);
  }
  return out;
}

}  // namespace foxhttp::router
