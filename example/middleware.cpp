#include <nodepp/nodepp.h>
#include <apify/apify.h>
#include <nodepp/ws.h>

/*────────────────────────────────────────────────────────────────────────────*/

using namespace nodepp;

/*────────────────────────────────────────────────────────────────────────────*/

void onMain() {

    auto app = apify::add<ws_t>();
    auto srv = ws::server();

    app.add( nullptr, [=]( apify_t<ws_t> cli, function_t<void> next ){

        console::log("Hello Im a Middleware !");
        next();

    });

    app.on( nullptr, nullptr, [=]( apify_t<ws_t> cli ){

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