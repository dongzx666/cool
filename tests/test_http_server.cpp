#include "src/address.h"
#include "src/http/http_server.h"
#include "src/http/http_session.h"
#include "src/config.h"

// LOGGER_DEF(g_logger, "root");

void run() {
  cool::http::HttpServer::ptr server(new cool::http::HttpServer);
  cool::Address::ptr addr = cool::Address::LookupAnyIPAddress("0.0.0.0:8020");
  while (!server->bind(addr)) {
    sleep(2);
  }
  auto sd = server->get_servlet_dispatch();
  sd->add_servlet("/cool/xx", [](cool::http::HttpRequest::ptr req,
                              cool::http::HttpResponse::ptr res,
                              cool::http::HttpSession::ptr session) {
    res->body(req->to_string());
    return 0;
  });
  sd->add_glob_servlet("/cool/*", [](cool::http::HttpRequest::ptr req,
                              cool::http::HttpResponse::ptr res,
                              cool::http::HttpSession::ptr session) {
    res->body("global\r\n" + req->to_string());
    return 0;
  });
  server->start();
}

int main(int argc, char *argv[]) {
  // static cool::Logger::ptr g_logger = LOG_NAME("system");
  // YAML::Node root = YAML::LoadFile("../config/log2.yml");
  // cool::Config::load_from_yaml(root);
  cool::IOManager iom(2);
  iom.schedule(run);
  return 0;
}
