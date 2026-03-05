/**
 * foxhttp - Advanced Authentication Middleware
 */

#pragma once

#include <boost/beast/http.hpp>
#include <foxhttp/middleware/middleware.hpp>
#include <foxhttp/server/request_context.hpp>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef FOXHTTP_ENABLE_JWT
#include <jwt-cpp/jwt.h>
#endif

namespace http = boost::beast::http;

namespace foxhttp {
namespace advanced {

struct auth_user {
  std::string id;
  std::vector<std::string> roles;
};

enum class auth_scheme {
  bearer_jwt,
  basic,
  digest,
  api_key,
  oauth2,
  session_cookie
};

class auth_middleware : public middleware {
 public:
  using jwt_validator = std::function<std::optional<auth_user>(const std::string &token)>;
  using basic_validator =
      std::function<std::optional<auth_user>(const std::string &username, const std::string &password)>;
  using digest_validator = std::function<std::optional<auth_user>(
      const std::unordered_map<std::string, std::string> &params, const std::string &method)>;
  using api_key_validator = std::function<std::optional<auth_user>(const std::string &api_key)>;
  using oauth2_validator = std::function<std::optional<auth_user>(const std::string &access_token)>;
  using session_lookup = std::function<std::optional<auth_user>(const std::string &session_id)>;

 public:
  explicit auth_middleware(std::vector<auth_scheme> schemes) : schemes_(std::move(schemes)) {}

  // Builder-style configuration
  auth_middleware &set_jwt_validator(jwt_validator v) {
    jwt_validator_ = std::move(v);
    return *this;
  }
  auth_middleware &set_basic_validator(basic_validator v) {
    basic_validator_ = std::move(v);
    return *this;
  }
  auth_middleware &set_digest_validator(digest_validator v) {
    digest_validator_ = std::move(v);
    return *this;
  }
  auth_middleware &set_api_key_validator(api_key_validator v) {
    api_key_validator_ = std::move(v);
    return *this;
  }
  auth_middleware &set_oauth2_validator(oauth2_validator v) {
    oauth2_validator_ = std::move(v);
    return *this;
  }
  auth_middleware &set_session_lookup(session_lookup v) {
    session_lookup_ = std::move(v);
    return *this;
  }

  auth_middleware &set_api_key_header(std::string name) {
    api_key_header_ = std::move(name);
    return *this;
  }
  auth_middleware &set_api_key_query_param(std::string name) {
    api_key_query_param_ = std::move(name);
    return *this;
  }
  auth_middleware &set_session_cookie_name(std::string name) {
    session_cookie_name_ = std::move(name);
    return *this;
  }
  auth_middleware &set_realm(std::string realm) {
    realm_ = std::move(realm);
    return *this;
  }

#ifdef FOXHTTP_ENABLE_JWT
  // Built-in jwt-cpp verification helpers (optional)
  auth_middleware &enable_jwt_hs256(const std::string &secret) {
    jwt_enabled_ = true;
    jwt_algo_ = jwt_algo::hs256;
    jwt_secret_ = secret;
    return *this;
  }
  auth_middleware &enable_jwt_rs256_public_key(const std::string &public_key_pem) {
    jwt_enabled_ = true;
    jwt_algo_ = jwt_algo::rs256;
    jwt_public_key_ = public_key_pem;
    return *this;
  }
  auth_middleware &set_jwt_issuer(std::string issuer) {
    jwt_issuer_ = std::move(issuer);
    return *this;
  }
  auth_middleware &set_jwt_audience(std::string audience) {
    jwt_audience_ = std::move(audience);
    return *this;
  }
#endif

  std::string name() const override { return "advanced_auth_middleware"; }
  middleware_priority priority() const override { return middleware_priority::high; }

  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next) override {
    try {
      for (auto scheme : schemes_) {
        auto user = authenticate(scheme, ctx);
        if (user.has_value()) {
          ctx.set<auth_user>("auth.user", *user);
          next();
          return;
        }
      }

      // Authentication failed
      res.result(http::status::unauthorized);
      res.set(http::field::content_type, "application/json");
      set_www_authenticate_header(res);
      res.body() = R"({"error":"unauthorized"})";
      res.prepare_payload();
    } catch (const std::exception &e) {
      res.result(http::status::unauthorized);
      res.set(http::field::content_type, "application/json");
      set_www_authenticate_header(res);
      res.body() = std::string("{\"error\":\"auth_error\",\"message\":\"") + e.what() + "\"}";
      res.prepare_payload();
    }
  }

 private:
  // Simple Base64 decode for Basic auth
  static std::string _base64_decode(const std::string &in) {
    static const int T[256] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, 52, 53, 54, 55,
        56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
        13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, 32,
        33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    std::string out;
    out.reserve(in.size() * 3 / 4);
    int val = 0, valb = -8;
    for (unsigned char c : in) {
      if (c == '=') break;
      int t = (c < 256) ? T[c] : -1;
      if (t == -1) continue;
      val = (val << 6) + t;
      valb += 6;
      if (valb >= 0) {
        out.push_back(char((val >> valb) & 0xFF));
        valb -= 8;
      }
    }
    return out;
  }

  static std::unordered_map<std::string, std::string> _parse_kv(const std::string &s) {
    std::unordered_map<std::string, std::string> out;
    std::string key, val;
    enum {
      K,
      V,
      Q
    } state = K;
    for (size_t i = 0; i < s.size(); ++i) {
      char c = s[i];
      if (state == K) {
        if (c == '=')
          state = V;
        else if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_')
          key.push_back(c);
      } else if (state == V) {
        if (c == '"') {
          state = Q;
        } else if (c == ',') {
          out[key] = val;
          key.clear();
          val.clear();
          state = K;
        } else if (!std::iscntrl(static_cast<unsigned char>(c)))
          val.push_back(c);
      } else if (state == Q) {
        if (c == '"') {
          state = V;
        } else if (c != '\r' && c != '\n')
          val.push_back(c);
      }
    }
    if (!key.empty()) out[key] = val;
    return out;
  }

  static std::optional<std::string> _query_param(const request_context &ctx, const std::string &key) {
    auto &q = ctx.query_parameters();
    auto it = q.find(key);
    if (it != q.end() && !it->second.empty()) return it->second.front();
    return std::nullopt;
  }

  static std::pair<std::string, std::string> _split_first(const std::string &s, char delim) {
    auto p = s.find(delim);
    if (p == std::string::npos) return {s, std::string()};
    return {s.substr(0, p), s.substr(p + 1)};
  }

  std::optional<auth_user> authenticate(auth_scheme scheme, request_context &ctx) const {
    switch (scheme) {
      case auth_scheme::bearer_jwt: {
        auto h = ctx.header(http::field::authorization);
        if (h.size() < 8 || h.rfind("Bearer ", 0) != 0) return std::nullopt;
        auto token = h.substr(7);
        if (token.size() > 8192) return std::nullopt;  // sanity limit
        if (jwt_validator_) return jwt_validator_(token);
#ifdef FOXHTTP_ENABLE_JWT
        if (jwt_enabled_) {
          try {
            auto decoded = jwt::decode(token);
            auto verifier = jwt::verify();
            verifier.leeway(5);  // small clock skew
            if (!jwt_issuer_.empty()) verifier.with_issuer(jwt_issuer_);
            if (!jwt_audience_.empty()) verifier.with_audience(jwt_audience_);
            switch (jwt_algo_) {
              case jwt_algo::hs256:
                verifier.allow_algorithm(jwt::algorithm::hs256{jwt_secret_});
                break;
              case jwt_algo::rs256:
                verifier.allow_algorithm(jwt::algorithm::rs256{jwt_public_key_, "", "", ""});
                break;
            }
            verifier.verify(decoded);

            std::string sub;
            std::vector<std::string> roles;
            if (decoded.has_payload_claim("sub")) sub = decoded.get_payload_claim("sub").as_string();
            if (decoded.has_payload_claim("roles")) {
              auto arr = decoded.get_payload_claim("roles").as_array();
              for (const auto &r : arr) roles.emplace_back(r.as_string());
            }
            if (sub.empty()) sub = "anonymous";
            return auth_user{sub, roles};
          } catch (...) {
            return std::nullopt;
          }
        }
#endif
        return std::nullopt;
      }
      case auth_scheme::basic: {
        if (!basic_validator_) return std::nullopt;
        auto h = ctx.header(http::field::authorization);
        if (h.rfind("Basic ", 0) == 0) {
          auto b64 = h.substr(6);
          auto decoded = _base64_decode(b64);
          auto pair = _split_first(decoded, ':');
          return basic_validator_(pair.first, pair.second);
        }
        return std::nullopt;
      }
      case auth_scheme::digest: {
        if (!digest_validator_) return std::nullopt;
        auto h = ctx.header(http::field::authorization);
        if (h.rfind("Digest ", 0) == 0) {
          auto params = _parse_kv(h.substr(7));
          return digest_validator_(params, ctx.method_string());
        }
        return std::nullopt;
      }
      case auth_scheme::api_key: {
        if (!api_key_validator_) return std::nullopt;
        // header first
        auto key = ctx.header(api_key_header_);
        if (key.empty()) {
          auto q = _query_param(ctx, api_key_query_param_);
          if (q.has_value()) key = *q;
        }
        if (!key.empty()) {
          if (key.size() > 256) return std::nullopt;  // sanity limit
          return api_key_validator_(key);
        }
        return std::nullopt;
      }
      case auth_scheme::oauth2: {
        if (!oauth2_validator_) return std::nullopt;
        auto h = ctx.header(http::field::authorization);
        if (h.rfind("Bearer ", 0) == 0) {
          auto token = h.substr(7);
          return oauth2_validator_(token);
        }
        return std::nullopt;
      }
      case auth_scheme::session_cookie: {
        if (!session_lookup_) return std::nullopt;
        auto sid = ctx.cookie(session_cookie_name_);
        if (!sid.empty()) return session_lookup_(sid);
        return std::nullopt;
      }
    }
    return std::nullopt;
  }

  void set_www_authenticate_header(http::response<http::string_body> &res) const {
    // Build combined WWW-Authenticate header for advertised schemes
    std::vector<std::string> parts;
    for (auto s : schemes_) {
      switch (s) {
        case auth_scheme::bearer_jwt:
        case auth_scheme::oauth2:
          parts.emplace_back("Bearer realm=\"" + realm_ + "\"");
          break;
        case auth_scheme::basic:
          parts.emplace_back("Basic realm=\"" + realm_ + "\"");
          break;
        case auth_scheme::digest:
          parts.emplace_back("Digest realm=\"" + realm_ + "\"");
          break;
        case auth_scheme::api_key:
          parts.emplace_back("ApiKey");
          break;
        case auth_scheme::session_cookie:
          // no standard header
          break;
      }
    }
    if (!parts.empty()) {
      std::string header;
      for (size_t i = 0; i < parts.size(); ++i) {
        if (i) header += ", ";
        header += parts[i];
      }
      res.set(http::field::www_authenticate, header);
    }
  }

 private:
  std::vector<auth_scheme> schemes_;
  std::string api_key_header_ = "X-API-Key";
  std::string api_key_query_param_ = "api_key";
  std::string session_cookie_name_ = "SESSIONID";
  std::string realm_ = "foxhttp";

  jwt_validator jwt_validator_;
  basic_validator basic_validator_;
  digest_validator digest_validator_;
  api_key_validator api_key_validator_;
  oauth2_validator oauth2_validator_;
  session_lookup session_lookup_;
};

}  // namespace advanced
}  // namespace foxhttp
