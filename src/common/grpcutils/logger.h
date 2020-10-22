#pragma once

#include <grpcpp/grpcpp.h>

namespace pl {

/**
 * The factory that creates a logger for a GRPC transaction.
 */
class LoggingInterceptorFactory : public grpc::experimental::ServerInterceptorFactoryInterface {
 public:
  grpc::experimental::Interceptor* CreateServerInterceptor(
      grpc::experimental::ServerRpcInfo* info) override;
};

}  // namespace pl