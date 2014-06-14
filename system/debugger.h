
#ifndef BSAL_DEBUGGER_H
#define BSAL_DEBUGGER_H

/*
 */

#define BSAL_DEBUG_MARKER(marker) \
        printf("BSAL_DEBUG_MARKER File %s, Line %d, Function %s, Marker %s\n", __FILE__, __LINE__, __func__, marker);

#endif
