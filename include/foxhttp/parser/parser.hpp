#pragma once

#include <boost/beast/http.hpp>
#include <memory>
#include <string>

namespace http = boost::beast::http;

namespace foxhttp {

template <typename T>
class parser {
 public:
  using result_type = T;
  virtual ~parser() = default;

  virtual std::string name() const = 0;
  virtual std::string content_type() const = 0;

  virtual bool supports(const http::request<http::string_body> &req) const = 0;
  virtual T parse(const http::request<http::string_body> &req) const = 0;

  virtual bool validate(const T &) const { return true; }

  virtual std::string error_message() const { return {}; }
};

class parser_holder_base {
 public:
  virtual ~parser_holder_base() = default;
  virtual std::string name() const = 0;
  virtual std::string content_type() const = 0;
  virtual bool supports(const http::request<http::string_body> &req) const = 0;
};

template <typename T>
class parser_holder : public parser_holder_base {
 public:
  explicit parser_holder(std::shared_ptr<parser<T>> impl) : impl_(std::move(impl)) {}
  std::string name() const override { return impl_->name(); }
  std::string content_type() const override { return impl_->content_type(); }
  bool supports(const http::request<http::string_body> &req) const override { return impl_->supports(req); }

  std::shared_ptr<parser<T>> get() const { return impl_; }

 private:
  std::shared_ptr<parser<T>> impl_;
};

class parser_factory {
 public:
  template <typename T>
  void register_parser(std::shared_ptr<parser<T>> parser) {
    auto key = std::type_index(typeid(T));
    auto &vec = registry_[key];
    vec.push_back(std::make_shared<parser_holder<T>>(std::move(parser)));
  }

  template <typename T>
  std::shared_ptr<parser<T>> get_best(const http::request<http::string_body> &req) const {
    auto it = registry_.find(std::type_index(typeid(T)));
    if (it == registry_.end()) throw std::runtime_error("No parser registered for this type");

    for (auto &holder_base : it->second) {
      auto holder = std::dynamic_pointer_cast<parser_holder<T>>(holder_base);
      if (holder && holder->supports(req)) return holder->get();
    }
    throw std::runtime_error("No suitable parser supports this request");
  }

  static parser_factory &instance() {
    static parser_factory inst;
    return inst;
  }

 private:
  std::unordered_map<std::type_index, std::vector<std::shared_ptr<parser_holder_base>>> registry_;
};
}  // namespace foxhttp

#define REGISTER_PARSER(TYPE, CLASSNAME)                                                        \
  namespace {                                                                                   \
  struct CLASSNAME##Registrar {                                                                 \
    CLASSNAME##Registrar() {                                                                    \
      foxhttp::parser_factory::instance().register_parser<TYPE>(std::make_shared<CLASSNAME>()); \
    }                                                                                           \
  };                                                                                            \
  static CLASSNAME##Registrar global_##CLASSNAME##_registrar;                                   \
  }
