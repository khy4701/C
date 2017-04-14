#include "mosnd.h"

int main(int ac, char **av){

    int     ret = 0;
    time_t  now, old = 0;

    // INIT VALUE
#if 0
    ret = initMosnd();
    if (ret < 0){
        fprintf(stderr, "[MOSND] Inital Fail");
        exit(1);
    }
#endif
    
    // LOOP
    now = time(0);
    old = now;
    while(1){

        now = time(0);
        if ( now != old )
        {
            old = now;

        }

    }

    return 0;
}
