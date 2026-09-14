#include "app.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>

void emscripten_loop(void* arg) 
{
    auto* app = static_cast<XboxStartup*>(arg);
    app->update();
}
#endif

int main(int argc, char** argv) 
{
    XboxStartup app(argc, argv);

#if defined(__EMSCRIPTEN__)
    emscripten_set_main_loop_arg(emscripten_loop, &app, 0, 1);
#else
    while (app.isRunning()) 
        app.update();
#endif

    return 0;
}