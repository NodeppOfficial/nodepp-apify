#include <nodepp/nodepp.h>
#include <apify/apify.h>
#include <nodepp/ws.h>

/*────────────────────────────────────────────────────────────────────────────*/

using namespace nodepp;

/*────────────────────────────────────────────────────────────────────────────*/

void onMain() {

    auto app = apify::add<ws_t>();
    auto srv = ws::server();

    app.on( "VALUE", "/:id/:value", [=]( apify_t<ws_t> cli ){

        console::log( cli.params["id"], cli.params["value"] );
        cli.emit( "DONE", nullptr, "hello world!" );

    });

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