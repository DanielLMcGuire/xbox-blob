#ifndef TOGGLE_MACRO_H
#define TOGGLE_MACRO_H

#define TOGGLE(x) do \
    { \
        if (x) \
            x = false; \
        else \
            x = true; \
    } while(0)

#endif