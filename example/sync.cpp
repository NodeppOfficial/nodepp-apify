#include <nodepp/nodepp.h>
#include <nodepp/encoder.h>
#include <nodepp/crypto.h>
#include <apify/apify.h>
#include <nodepp/wait.h>
#include <nodepp/ws.h>

using namespace nodepp;

wait_t<string_t,bool,string_t> onResponse;

promise_t<string_t,except_t> request( ws_t cli ) {
return promise_t<string_t,except_t>([=]( res_t<string_t> res, rej_t<except_t> rej ){
        
    auto sha = crypto::hash::SHA1(); 
    sha.update( encoder::key::generate( 32 ) );
    sha.update( string::to_string( process::now() ) );
    
    auto pid = regex::format( "/${0}", sha.get() );
    
    onResponse.once( pid, [=]( bool fail, string_t message ){
        
        switch( fail ){
            case false: res( message ); /*-------*/ break;
            default   : rej( except_t( message ) ); break;
        }
        
    });
    
    apify::add( cli ).emit( "METHOD", pid, "MESSAGE" );
        
}); }

void client(){
    auto srv = ws::client( "ws://localhost:8000" );
    auto app = apify::add<ws_t>();
    
    app.on( "DONE", "/:pid", [=]( apify_t<ws_t> cli ){
        
        auto pid=regex::format("/${0}",cli.params["pid"]);
        onResponse.emit( pid, 0, cli.message );    
        
    });
    
    app.on( "FAIL", "/:pid", [=]( apify_t<ws_t> cli ){ 
        
        auto pid=regex::format("/${0}",cli.params["pid"]);
        onResponse.emit( pid, 1, cli.message );    
        
    });

    srv.onConnect([=]( ws_t cli ){

        cli.onData([=]( string_t data ){
            app.next( cli, data );
        });

        srv.onClose([=](){
            console::log("Disconnected");
        }); console::log("Connected");

        request( cli ).then([=]( string_t message ){
            console::log( "->", message );
        }).fail([=]( except_t err ){
            console::log( "->", err.data() );
        });

    });

}

void server() {
    auto app = apify::add<ws_t>();
    auto srv = ws::server();

    app.on( "METHOD", "/:pid", [=]( apify_t<ws_t> cli ){ 
    auto pid=regex::format("/${0}",cli.params["pid"]) ; try {
    
        cli.emit( "DONE", pid, "Message received !" );
        process::exit(1);
    
    } catch(...) {
    
        cli.emit( "FAIL", pid, "something went wrong" );
        process::exit(1);
    
    } });

    srv.onConnect([=]( ws_t cli ){

        cli.onData([=]( string_t data ){
            app.next( cli, data );
        });

        cli.onClose([=](){
            console::log("Disconnected");
        }); console::log("Connected");

    });

    srv.listen("localhost",8000,[=](...){
        console::log( "ws:/localhost:8000" );
    }); console::log( "Started" );

}

void onMain() {

    if( process::env::get("mode") == "server" )
      { server(); } else { client(); }

}