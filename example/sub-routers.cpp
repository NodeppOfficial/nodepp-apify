#include <nodepp/nodepp.h>
#include <apify/apify.h>
#include <nodepp/ws.h>

/*────────────────────────────────────────────────────────────────────────────*/

using namespace nodepp;

/*────────────────────────────────────────────────────────────────────────────*/

apify_host_t<ws_t> sub_router(){
    auto app = apify::add<ws_t>();

    app.on("METHOD","/path",[=]( apify_t<ws_t> cli ){
        console::log( cli.message );
        cli.emit( "DONE", nullptr, "this is a sub-router" );
    });

    return app;
}

/*────────────────────────────────────────────────────────────────────────────*/

void onMain() {

    auto app = apify::add<ws_t>();
    auto srv = ws::server();

    app.add( "/sub", sub_router() );

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