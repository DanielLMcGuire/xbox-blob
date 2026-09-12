#include "app.h"

int main(int argc, char** argv) 
{
    XboxStartup app(argc, argv);

    while (app.isRunning()) 
        app.update();

    return 0;
}