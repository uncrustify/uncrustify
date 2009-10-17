/* traffic light synchronizer, using states in an event-driven model */
#include <time>
main()                                state green_wait
@keypressed(key) <green_wait>         state yellow_wait
@keypressed(key) <red_walk, red_wait> state red_walk
@keypressed(key) <>                   {} /* fallback */
@timer()         <yellow_wait>        state red_walk
@timer()         <red_walk>           state red_wait
@timer()         <red_wait>           state green_wait
@timer()         <>                   {} /* fallback */
entry() <green_wait>
    print "Green / Don't walk\n"
entry() <yellow_wait>
    {
    print "Yellow / Don't walk\n"
    settimer 2000
    }
entry() <red_walk>
    {
    print "Red / Walk\n"
    settimer 5000
    }
entry() <red_wait>
    {
    print "Red / Don't walk\n"
    settimer 2000
    }

