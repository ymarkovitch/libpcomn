/*-*- mode: c++; tab-width: 4; c-file-style: "stroustrup"; c-file-offsets:((innamespace . 0) (inline-open . 0) (case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   test_http_server01.cpp

 DESCRIPTION  :   HTTP echo server.
                  This server responds to a GET request sending response with the text of
                  incoming request as response body.

 CREATION DATE:   10 Mar 2008
*******************************************************************************/
#include <pcomn_http/http_basic_server.h>
#include <pcomn_syncobj.h>
#include <pcomn_trace.h>

using namespace pcomn ;
using namespace pcomn::http ;
using namespace pcomn::svr ;

using pcomn::StreamLock ;

class HTTPEchoServerSession : public HTTPServerSession {
      typedef HTTPServerSession ancestor ;
public:
    explicit HTTPEchoServerSession(connection_ptr connection) :
        ancestor(connection)
    {}
private:
    // Implement abstract virtual function run()
    int run() ;
} ;

int HTTPEchoServerSession::run()
{
    TRACEP("Running HTTPEchoServerSession on connection " << connection()) ;
    StreamLock(std::cout).stream() << "HTTPEchoServerSession was connected to " << connection().peer() << std::endl ;

    try {
        // Note this loop. The server processes requests in a loop one by one until either
        // client requests close, or an error occurs.
        do {
            // We needn't any request content anyway, so skip it if exists.
            const HTTPRequest &request = http().receive_request(true) ;

            // GET is the _only_ method allowed.
            if (request.method() == HTTPRequest::HTTP_GET)
                // Respond with the request content.
                http().respond(HTTP_RSP_OK, request.str(pcomn::eolmode_LF)) ;

            else
            {
                // Answer error 405 and set 'Connection: close'
                HTTPResponse response (HTTP_RSP_METHOD_NOT_ALLOWED | HTTP_RSPFLAG_CLOSE) ;
                // Be polite, say what _is_ allowed.
                response.set_header("allow", "GET") ;
                http().respond(response) ;
            }
        }
        // Run until connection is closed
        while(!http().is_closed()) ;

        StreamLock(std::cout).stream() << "Closing connection " << http().id() << " in normal fashion." << std::endl ;
    }
    catch(const std::exception &x)
    {
      StreamLock(std::cout).stream() << "Exception in connection: " << x.what() << std::endl ;
      throw ;
   }

    return 1 ;
}

typedef HTTPBasicServer<HTTPEchoServerSession> HTTPEchoServer ;

int main(int argc, char *argv[])
{
   DIAG_INITTRACE("httptest.ini") ;

   try
   {
      int port = 50080 ;
      int backlog = 30 ;
      int pending_q = 30 ;

      HTTPEchoServer server("HTTP Echo Test Server/0.1", "text/plain", 2, 5) ;
      net::sock_address address(argc > 1 ? argv[1] : "localhost", argc > 2 ? (port = atoi(argv[2])) : port) ;

      StreamLock(std::cout).stream() << "Starting server(" << address
                                     << ", " << backlog << ", " << pending_q << ')' << std::endl ;

      if (!server.start(address, backlog, pending_q))
      {
         StreamLock(std::cout).stream() << "Server hasn't started" << std::endl ;
         return 1 ;
      }

      char c ;
      StreamLock(std::cout).stream() << "Server has started\nPlease hit <AnyKey><ENTER> to exit." << std::endl ;
      std::cin >> c ;
      StreamLock(std::cout).stream() << "Crash-stopping the server..." << std::endl ;
      server.stop(0) ;
      StreamLock(std::cout).stream() << "The server has stopped." << std::endl ;

   }
   catch(const std::exception &x)
   {
      StreamLock(std::cout).stream() << "Exception: " << x.what() << std::endl ;
      return 1 ;
   }
   return 0 ;
}
