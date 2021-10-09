#include "src/http/http.h"
#include "src/log.h"
#include <iostream>


void test_request () {
  cool::http::HttpRequest::ptr req(new cool::http::HttpRequest);
  req->setHeader("host", "www.baidu.com");
  req->body("hello baidu");

  req->dump(std::cout) << std::endl;
}

void test_response () {
  cool::http::HttpResponse::ptr rsp(new cool::http::HttpResponse);
  rsp->setHeader("X-X", "baidu");
  rsp->status((cool::http::http_status)400);
  rsp->setClose(false);
  rsp->body("hello baidu");

  rsp->dump(std::cout) << std::endl;
}

int main(int argc, char* argv[]) {
  test_request();
  test_response();
  return 0;
}
