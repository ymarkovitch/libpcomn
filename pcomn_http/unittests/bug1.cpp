#include <pcomn_http/http_connection.h>
#include <pcomn_csvr/commsvr_blocking_connection.h>

#include <iostream>
#include <stdexcept>

using namespace pcomn::net;
using namespace pcomn::svr;
using namespace pcomn::http;

void query( const char *a_url, HTTPClientConnection &a_conn )
{
    std::cout << "quering " << a_url << std::endl;
    HTTPRequest message( HTTPRequest::HTTP_GET, URI( a_url ) );
    a_conn.request( message, NULL, 0 );
    /*const HTTPResponse &response =*/ a_conn.receive_response( true /* skip rest of data*/ );
}

int main()
{
    DIAG_INITTRACE("bug1.ini") ;
    BlockingConnection      connection( sock_address( inet_address( "localhost" ), 8000 ) );
    HTTPClientConnection    http( connection, HTTPMessage::MSGFKeepAlive );

    query( "http://localhost/", http );
    query( "http://localhost/", http );

    return 0;
}
