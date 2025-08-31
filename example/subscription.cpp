#include <nodepp/nodepp.h>
#include <apify/apify.h>
#include <nodepp/ws.h>

/*────────────────────────────────────────────────────────────────────────────*/

using namespace nodepp;

/*────────────────────────────────────────────────────────────────────────────*/

void onMain() {

    auto app = apify::add<ws_t>();
    auto srv = ws::server();
    queue_t<ws_t> clients;

    app.on( "SUBS", nullptr, [=]( apify_t<ws_t> cli ){

        clients.push( cli.get_fd() );

    });

    process::add( coroutine::add( COROUTINE(){
    coBegin ; coDelay( 1000 );
    
        do{ auto x = clients.first(); while( x != nullptr ){
            auto y = x->next; 
            
        if( x->data.is_closed() ){ clients.erase(x); x=y; continue; }
            
            auto message = regex::format( "-> ${0}", process::now() );
            apify::add( x->data ).emit( "DONE", nullptr, message );
        
        x=y; }} while(0); coGoto(0);
    
    coFinish
    }));

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

/*────────────────────────────────────────────────────────────────────────────*/