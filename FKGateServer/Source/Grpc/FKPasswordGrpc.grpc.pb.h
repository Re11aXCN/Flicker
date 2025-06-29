// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: FKPasswordGrpc.proto
#ifndef GRPC_FKPasswordGrpc_2eproto__INCLUDED
#define GRPC_FKPasswordGrpc_2eproto__INCLUDED

#include "FKPasswordGrpc.pb.h"

#include <functional>
#include <grpcpp/generic/async_generic_service.h>
#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/async_unary_call.h>
#include <grpcpp/support/client_callback.h>
#include <grpcpp/client_context.h>
#include <grpcpp/completion_queue.h>
#include <grpcpp/support/message_allocator.h>
#include <grpcpp/support/method_handler.h>
#include <grpcpp/impl/proto_utils.h>
#include <grpcpp/impl/rpc_method.h>
#include <grpcpp/support/server_callback.h>
#include <grpcpp/impl/server_callback_handlers.h>
#include <grpcpp/server_context.h>
#include <grpcpp/impl/service_type.h>
#include <grpcpp/support/status.h>
#include <grpcpp/support/stub_options.h>
#include <grpcpp/support/sync_stream.h>
#include <grpcpp/ports_def.inc>

namespace FKPasswordGrpc {

class PasswordService final {
 public:
  static constexpr char const* service_full_name() {
    return "FKPasswordGrpc.PasswordService";
  }
  class StubInterface {
   public:
    virtual ~StubInterface() {}
    virtual ::grpc::Status HashPassword(::grpc::ClientContext* context, const ::FKPasswordGrpc::HashPasswordRequest& request, ::FKPasswordGrpc::HashPasswordResponse* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::FKPasswordGrpc::HashPasswordResponse>> AsyncHashPassword(::grpc::ClientContext* context, const ::FKPasswordGrpc::HashPasswordRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::FKPasswordGrpc::HashPasswordResponse>>(AsyncHashPasswordRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::FKPasswordGrpc::HashPasswordResponse>> PrepareAsyncHashPassword(::grpc::ClientContext* context, const ::FKPasswordGrpc::HashPasswordRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::FKPasswordGrpc::HashPasswordResponse>>(PrepareAsyncHashPasswordRaw(context, request, cq));
    }
    virtual ::grpc::Status VerifyPassword(::grpc::ClientContext* context, const ::FKPasswordGrpc::VerifyPasswordRequest& request, ::FKPasswordGrpc::VerifyPasswordResponse* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::FKPasswordGrpc::VerifyPasswordResponse>> AsyncVerifyPassword(::grpc::ClientContext* context, const ::FKPasswordGrpc::VerifyPasswordRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::FKPasswordGrpc::VerifyPasswordResponse>>(AsyncVerifyPasswordRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::FKPasswordGrpc::VerifyPasswordResponse>> PrepareAsyncVerifyPassword(::grpc::ClientContext* context, const ::FKPasswordGrpc::VerifyPasswordRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::FKPasswordGrpc::VerifyPasswordResponse>>(PrepareAsyncVerifyPasswordRaw(context, request, cq));
    }
    class async_interface {
     public:
      virtual ~async_interface() {}
      virtual void HashPassword(::grpc::ClientContext* context, const ::FKPasswordGrpc::HashPasswordRequest* request, ::FKPasswordGrpc::HashPasswordResponse* response, std::function<void(::grpc::Status)>) = 0;
      virtual void HashPassword(::grpc::ClientContext* context, const ::FKPasswordGrpc::HashPasswordRequest* request, ::FKPasswordGrpc::HashPasswordResponse* response, ::grpc::ClientUnaryReactor* reactor) = 0;
      virtual void VerifyPassword(::grpc::ClientContext* context, const ::FKPasswordGrpc::VerifyPasswordRequest* request, ::FKPasswordGrpc::VerifyPasswordResponse* response, std::function<void(::grpc::Status)>) = 0;
      virtual void VerifyPassword(::grpc::ClientContext* context, const ::FKPasswordGrpc::VerifyPasswordRequest* request, ::FKPasswordGrpc::VerifyPasswordResponse* response, ::grpc::ClientUnaryReactor* reactor) = 0;
    };
    typedef class async_interface experimental_async_interface;
    virtual class async_interface* async() { return nullptr; }
    class async_interface* experimental_async() { return async(); }
   private:
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::FKPasswordGrpc::HashPasswordResponse>* AsyncHashPasswordRaw(::grpc::ClientContext* context, const ::FKPasswordGrpc::HashPasswordRequest& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::FKPasswordGrpc::HashPasswordResponse>* PrepareAsyncHashPasswordRaw(::grpc::ClientContext* context, const ::FKPasswordGrpc::HashPasswordRequest& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::FKPasswordGrpc::VerifyPasswordResponse>* AsyncVerifyPasswordRaw(::grpc::ClientContext* context, const ::FKPasswordGrpc::VerifyPasswordRequest& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::FKPasswordGrpc::VerifyPasswordResponse>* PrepareAsyncVerifyPasswordRaw(::grpc::ClientContext* context, const ::FKPasswordGrpc::VerifyPasswordRequest& request, ::grpc::CompletionQueue* cq) = 0;
  };
  class Stub final : public StubInterface {
   public:
    Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());
    ::grpc::Status HashPassword(::grpc::ClientContext* context, const ::FKPasswordGrpc::HashPasswordRequest& request, ::FKPasswordGrpc::HashPasswordResponse* response) override;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::FKPasswordGrpc::HashPasswordResponse>> AsyncHashPassword(::grpc::ClientContext* context, const ::FKPasswordGrpc::HashPasswordRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::FKPasswordGrpc::HashPasswordResponse>>(AsyncHashPasswordRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::FKPasswordGrpc::HashPasswordResponse>> PrepareAsyncHashPassword(::grpc::ClientContext* context, const ::FKPasswordGrpc::HashPasswordRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::FKPasswordGrpc::HashPasswordResponse>>(PrepareAsyncHashPasswordRaw(context, request, cq));
    }
    ::grpc::Status VerifyPassword(::grpc::ClientContext* context, const ::FKPasswordGrpc::VerifyPasswordRequest& request, ::FKPasswordGrpc::VerifyPasswordResponse* response) override;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::FKPasswordGrpc::VerifyPasswordResponse>> AsyncVerifyPassword(::grpc::ClientContext* context, const ::FKPasswordGrpc::VerifyPasswordRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::FKPasswordGrpc::VerifyPasswordResponse>>(AsyncVerifyPasswordRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::FKPasswordGrpc::VerifyPasswordResponse>> PrepareAsyncVerifyPassword(::grpc::ClientContext* context, const ::FKPasswordGrpc::VerifyPasswordRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::FKPasswordGrpc::VerifyPasswordResponse>>(PrepareAsyncVerifyPasswordRaw(context, request, cq));
    }
    class async final :
      public StubInterface::async_interface {
     public:
      void HashPassword(::grpc::ClientContext* context, const ::FKPasswordGrpc::HashPasswordRequest* request, ::FKPasswordGrpc::HashPasswordResponse* response, std::function<void(::grpc::Status)>) override;
      void HashPassword(::grpc::ClientContext* context, const ::FKPasswordGrpc::HashPasswordRequest* request, ::FKPasswordGrpc::HashPasswordResponse* response, ::grpc::ClientUnaryReactor* reactor) override;
      void VerifyPassword(::grpc::ClientContext* context, const ::FKPasswordGrpc::VerifyPasswordRequest* request, ::FKPasswordGrpc::VerifyPasswordResponse* response, std::function<void(::grpc::Status)>) override;
      void VerifyPassword(::grpc::ClientContext* context, const ::FKPasswordGrpc::VerifyPasswordRequest* request, ::FKPasswordGrpc::VerifyPasswordResponse* response, ::grpc::ClientUnaryReactor* reactor) override;
     private:
      friend class Stub;
      explicit async(Stub* stub): stub_(stub) { }
      Stub* stub() { return stub_; }
      Stub* stub_;
    };
    class async* async() override { return &async_stub_; }

   private:
    std::shared_ptr< ::grpc::ChannelInterface> channel_;
    class async async_stub_{this};
    ::grpc::ClientAsyncResponseReader< ::FKPasswordGrpc::HashPasswordResponse>* AsyncHashPasswordRaw(::grpc::ClientContext* context, const ::FKPasswordGrpc::HashPasswordRequest& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::FKPasswordGrpc::HashPasswordResponse>* PrepareAsyncHashPasswordRaw(::grpc::ClientContext* context, const ::FKPasswordGrpc::HashPasswordRequest& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::FKPasswordGrpc::VerifyPasswordResponse>* AsyncVerifyPasswordRaw(::grpc::ClientContext* context, const ::FKPasswordGrpc::VerifyPasswordRequest& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::FKPasswordGrpc::VerifyPasswordResponse>* PrepareAsyncVerifyPasswordRaw(::grpc::ClientContext* context, const ::FKPasswordGrpc::VerifyPasswordRequest& request, ::grpc::CompletionQueue* cq) override;
    const ::grpc::internal::RpcMethod rpcmethod_HashPassword_;
    const ::grpc::internal::RpcMethod rpcmethod_VerifyPassword_;
  };
  static std::unique_ptr<Stub> NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());

  class Service : public ::grpc::Service {
   public:
    Service();
    virtual ~Service();
    virtual ::grpc::Status HashPassword(::grpc::ServerContext* context, const ::FKPasswordGrpc::HashPasswordRequest* request, ::FKPasswordGrpc::HashPasswordResponse* response);
    virtual ::grpc::Status VerifyPassword(::grpc::ServerContext* context, const ::FKPasswordGrpc::VerifyPasswordRequest* request, ::FKPasswordGrpc::VerifyPasswordResponse* response);
  };
  template <class BaseClass>
  class WithAsyncMethod_HashPassword : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithAsyncMethod_HashPassword() {
      ::grpc::Service::MarkMethodAsync(0);
    }
    ~WithAsyncMethod_HashPassword() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status HashPassword(::grpc::ServerContext* /*context*/, const ::FKPasswordGrpc::HashPasswordRequest* /*request*/, ::FKPasswordGrpc::HashPasswordResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestHashPassword(::grpc::ServerContext* context, ::FKPasswordGrpc::HashPasswordRequest* request, ::grpc::ServerAsyncResponseWriter< ::FKPasswordGrpc::HashPasswordResponse>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithAsyncMethod_VerifyPassword : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithAsyncMethod_VerifyPassword() {
      ::grpc::Service::MarkMethodAsync(1);
    }
    ~WithAsyncMethod_VerifyPassword() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status VerifyPassword(::grpc::ServerContext* /*context*/, const ::FKPasswordGrpc::VerifyPasswordRequest* /*request*/, ::FKPasswordGrpc::VerifyPasswordResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestVerifyPassword(::grpc::ServerContext* context, ::FKPasswordGrpc::VerifyPasswordRequest* request, ::grpc::ServerAsyncResponseWriter< ::FKPasswordGrpc::VerifyPasswordResponse>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(1, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  typedef WithAsyncMethod_HashPassword<WithAsyncMethod_VerifyPassword<Service > > AsyncService;
  template <class BaseClass>
  class WithCallbackMethod_HashPassword : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithCallbackMethod_HashPassword() {
      ::grpc::Service::MarkMethodCallback(0,
          new ::grpc::internal::CallbackUnaryHandler< ::FKPasswordGrpc::HashPasswordRequest, ::FKPasswordGrpc::HashPasswordResponse>(
            [this](
                   ::grpc::CallbackServerContext* context, const ::FKPasswordGrpc::HashPasswordRequest* request, ::FKPasswordGrpc::HashPasswordResponse* response) { return this->HashPassword(context, request, response); }));}
    void SetMessageAllocatorFor_HashPassword(
        ::grpc::MessageAllocator< ::FKPasswordGrpc::HashPasswordRequest, ::FKPasswordGrpc::HashPasswordResponse>* allocator) {
      ::grpc::internal::MethodHandler* const handler = ::grpc::Service::GetHandler(0);
      static_cast<::grpc::internal::CallbackUnaryHandler< ::FKPasswordGrpc::HashPasswordRequest, ::FKPasswordGrpc::HashPasswordResponse>*>(handler)
              ->SetMessageAllocator(allocator);
    }
    ~WithCallbackMethod_HashPassword() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status HashPassword(::grpc::ServerContext* /*context*/, const ::FKPasswordGrpc::HashPasswordRequest* /*request*/, ::FKPasswordGrpc::HashPasswordResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual ::grpc::ServerUnaryReactor* HashPassword(
      ::grpc::CallbackServerContext* /*context*/, const ::FKPasswordGrpc::HashPasswordRequest* /*request*/, ::FKPasswordGrpc::HashPasswordResponse* /*response*/)  { return nullptr; }
  };
  template <class BaseClass>
  class WithCallbackMethod_VerifyPassword : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithCallbackMethod_VerifyPassword() {
      ::grpc::Service::MarkMethodCallback(1,
          new ::grpc::internal::CallbackUnaryHandler< ::FKPasswordGrpc::VerifyPasswordRequest, ::FKPasswordGrpc::VerifyPasswordResponse>(
            [this](
                   ::grpc::CallbackServerContext* context, const ::FKPasswordGrpc::VerifyPasswordRequest* request, ::FKPasswordGrpc::VerifyPasswordResponse* response) { return this->VerifyPassword(context, request, response); }));}
    void SetMessageAllocatorFor_VerifyPassword(
        ::grpc::MessageAllocator< ::FKPasswordGrpc::VerifyPasswordRequest, ::FKPasswordGrpc::VerifyPasswordResponse>* allocator) {
      ::grpc::internal::MethodHandler* const handler = ::grpc::Service::GetHandler(1);
      static_cast<::grpc::internal::CallbackUnaryHandler< ::FKPasswordGrpc::VerifyPasswordRequest, ::FKPasswordGrpc::VerifyPasswordResponse>*>(handler)
              ->SetMessageAllocator(allocator);
    }
    ~WithCallbackMethod_VerifyPassword() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status VerifyPassword(::grpc::ServerContext* /*context*/, const ::FKPasswordGrpc::VerifyPasswordRequest* /*request*/, ::FKPasswordGrpc::VerifyPasswordResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual ::grpc::ServerUnaryReactor* VerifyPassword(
      ::grpc::CallbackServerContext* /*context*/, const ::FKPasswordGrpc::VerifyPasswordRequest* /*request*/, ::FKPasswordGrpc::VerifyPasswordResponse* /*response*/)  { return nullptr; }
  };
  typedef WithCallbackMethod_HashPassword<WithCallbackMethod_VerifyPassword<Service > > CallbackService;
  typedef CallbackService ExperimentalCallbackService;
  template <class BaseClass>
  class WithGenericMethod_HashPassword : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithGenericMethod_HashPassword() {
      ::grpc::Service::MarkMethodGeneric(0);
    }
    ~WithGenericMethod_HashPassword() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status HashPassword(::grpc::ServerContext* /*context*/, const ::FKPasswordGrpc::HashPasswordRequest* /*request*/, ::FKPasswordGrpc::HashPasswordResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithGenericMethod_VerifyPassword : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithGenericMethod_VerifyPassword() {
      ::grpc::Service::MarkMethodGeneric(1);
    }
    ~WithGenericMethod_VerifyPassword() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status VerifyPassword(::grpc::ServerContext* /*context*/, const ::FKPasswordGrpc::VerifyPasswordRequest* /*request*/, ::FKPasswordGrpc::VerifyPasswordResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithRawMethod_HashPassword : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawMethod_HashPassword() {
      ::grpc::Service::MarkMethodRaw(0);
    }
    ~WithRawMethod_HashPassword() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status HashPassword(::grpc::ServerContext* /*context*/, const ::FKPasswordGrpc::HashPasswordRequest* /*request*/, ::FKPasswordGrpc::HashPasswordResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestHashPassword(::grpc::ServerContext* context, ::grpc::ByteBuffer* request, ::grpc::ServerAsyncResponseWriter< ::grpc::ByteBuffer>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithRawMethod_VerifyPassword : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawMethod_VerifyPassword() {
      ::grpc::Service::MarkMethodRaw(1);
    }
    ~WithRawMethod_VerifyPassword() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status VerifyPassword(::grpc::ServerContext* /*context*/, const ::FKPasswordGrpc::VerifyPasswordRequest* /*request*/, ::FKPasswordGrpc::VerifyPasswordResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestVerifyPassword(::grpc::ServerContext* context, ::grpc::ByteBuffer* request, ::grpc::ServerAsyncResponseWriter< ::grpc::ByteBuffer>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(1, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithRawCallbackMethod_HashPassword : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawCallbackMethod_HashPassword() {
      ::grpc::Service::MarkMethodRawCallback(0,
          new ::grpc::internal::CallbackUnaryHandler< ::grpc::ByteBuffer, ::grpc::ByteBuffer>(
            [this](
                   ::grpc::CallbackServerContext* context, const ::grpc::ByteBuffer* request, ::grpc::ByteBuffer* response) { return this->HashPassword(context, request, response); }));
    }
    ~WithRawCallbackMethod_HashPassword() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status HashPassword(::grpc::ServerContext* /*context*/, const ::FKPasswordGrpc::HashPasswordRequest* /*request*/, ::FKPasswordGrpc::HashPasswordResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual ::grpc::ServerUnaryReactor* HashPassword(
      ::grpc::CallbackServerContext* /*context*/, const ::grpc::ByteBuffer* /*request*/, ::grpc::ByteBuffer* /*response*/)  { return nullptr; }
  };
  template <class BaseClass>
  class WithRawCallbackMethod_VerifyPassword : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawCallbackMethod_VerifyPassword() {
      ::grpc::Service::MarkMethodRawCallback(1,
          new ::grpc::internal::CallbackUnaryHandler< ::grpc::ByteBuffer, ::grpc::ByteBuffer>(
            [this](
                   ::grpc::CallbackServerContext* context, const ::grpc::ByteBuffer* request, ::grpc::ByteBuffer* response) { return this->VerifyPassword(context, request, response); }));
    }
    ~WithRawCallbackMethod_VerifyPassword() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status VerifyPassword(::grpc::ServerContext* /*context*/, const ::FKPasswordGrpc::VerifyPasswordRequest* /*request*/, ::FKPasswordGrpc::VerifyPasswordResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual ::grpc::ServerUnaryReactor* VerifyPassword(
      ::grpc::CallbackServerContext* /*context*/, const ::grpc::ByteBuffer* /*request*/, ::grpc::ByteBuffer* /*response*/)  { return nullptr; }
  };
  template <class BaseClass>
  class WithStreamedUnaryMethod_HashPassword : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithStreamedUnaryMethod_HashPassword() {
      ::grpc::Service::MarkMethodStreamed(0,
        new ::grpc::internal::StreamedUnaryHandler<
          ::FKPasswordGrpc::HashPasswordRequest, ::FKPasswordGrpc::HashPasswordResponse>(
            [this](::grpc::ServerContext* context,
                   ::grpc::ServerUnaryStreamer<
                     ::FKPasswordGrpc::HashPasswordRequest, ::FKPasswordGrpc::HashPasswordResponse>* streamer) {
                       return this->StreamedHashPassword(context,
                         streamer);
                  }));
    }
    ~WithStreamedUnaryMethod_HashPassword() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable regular version of this method
    ::grpc::Status HashPassword(::grpc::ServerContext* /*context*/, const ::FKPasswordGrpc::HashPasswordRequest* /*request*/, ::FKPasswordGrpc::HashPasswordResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    // replace default version of method with streamed unary
    virtual ::grpc::Status StreamedHashPassword(::grpc::ServerContext* context, ::grpc::ServerUnaryStreamer< ::FKPasswordGrpc::HashPasswordRequest,::FKPasswordGrpc::HashPasswordResponse>* server_unary_streamer) = 0;
  };
  template <class BaseClass>
  class WithStreamedUnaryMethod_VerifyPassword : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithStreamedUnaryMethod_VerifyPassword() {
      ::grpc::Service::MarkMethodStreamed(1,
        new ::grpc::internal::StreamedUnaryHandler<
          ::FKPasswordGrpc::VerifyPasswordRequest, ::FKPasswordGrpc::VerifyPasswordResponse>(
            [this](::grpc::ServerContext* context,
                   ::grpc::ServerUnaryStreamer<
                     ::FKPasswordGrpc::VerifyPasswordRequest, ::FKPasswordGrpc::VerifyPasswordResponse>* streamer) {
                       return this->StreamedVerifyPassword(context,
                         streamer);
                  }));
    }
    ~WithStreamedUnaryMethod_VerifyPassword() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable regular version of this method
    ::grpc::Status VerifyPassword(::grpc::ServerContext* /*context*/, const ::FKPasswordGrpc::VerifyPasswordRequest* /*request*/, ::FKPasswordGrpc::VerifyPasswordResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    // replace default version of method with streamed unary
    virtual ::grpc::Status StreamedVerifyPassword(::grpc::ServerContext* context, ::grpc::ServerUnaryStreamer< ::FKPasswordGrpc::VerifyPasswordRequest,::FKPasswordGrpc::VerifyPasswordResponse>* server_unary_streamer) = 0;
  };
  typedef WithStreamedUnaryMethod_HashPassword<WithStreamedUnaryMethod_VerifyPassword<Service > > StreamedUnaryService;
  typedef Service SplitStreamedService;
  typedef WithStreamedUnaryMethod_HashPassword<WithStreamedUnaryMethod_VerifyPassword<Service > > StreamedService;
};

}  // namespace FKPasswordGrpc


#include <grpcpp/ports_undef.inc>
#endif  // GRPC_FKPasswordGrpc_2eproto__INCLUDED
