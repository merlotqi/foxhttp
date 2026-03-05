/**
 * foxhttp - TLS config helpers
 */

#pragma once

#include <boost/asio/ssl.hpp>
#include <string>

namespace foxhttp::tls {

inline void load_server_certificate(boost::asio::ssl::context &ctx, const std::string &cert_chain_file,
                                    const std::string &private_key_file, const std::string &dh_file = std::string()) {
  using boost::asio::ssl::context;

  ctx.set_options(context::default_workarounds | context::no_sslv2 | context::no_sslv3 | context::single_dh_use);

  ctx.use_certificate_chain_file(cert_chain_file);
  ctx.use_private_key_file(private_key_file, context::file_format::pem);
  if (!dh_file.empty()) {
    ctx.use_tmp_dh_file(dh_file);
  }
}

}  // namespace foxhttp::tls
