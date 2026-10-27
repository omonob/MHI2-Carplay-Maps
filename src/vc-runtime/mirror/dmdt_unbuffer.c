#include <stdio.h>
/* Diagnostic child only, never preload into HMI or DisplayManager. */
__attribute__((constructor)) static void unbuffer_dmdt_output(void)
{
    setvbuf(stdout,NULL,_IONBF,0);
    setvbuf(stderr,NULL,_IONBF,0);
}
