void Launcher::signal(int code)
{
    /*
    1 HUP      2 INT      3 QUIT     4 ILL      5 TRAP     6 ABRT     7 BUS
    8 FPE      9 KILL    10 USR1    11 SEGV    12 USR2    13 PIPE    14 ALRM
    15 TERM    16 STKFLT  17 CHLD    18 CONT    19 STOP    20 TSTP    21 TTIN
    22 TTOU    23 URG     24 XCPU    25 XFSZ    26 VTALRM  27 PROF    28 WINCH
    29 POLL    30 PWR     31 SYS


        Operation          WinCode     NixCode
        Status             128          1 (HUP)
        Terminate          N/A          2 (INT) Linux or macOS uses this for CTRL-C.
                        129          3
                        130          4
                        131          5
                        132          6
                        133          7
                                        8
                                        9
                                    10
                                    11
                                    12
                                    13
                                    14
        Terminate          N/A         15 (TERM) Linux or macOS uses this for CTRL-C.
                                    16
                       N/A         17 (CHILD) Child process exited.
                        N/A         28 WINCH, window changed size.
        */

    // Convert to lower range
    if (code >= 128)
    {
        code -= 127;
    }

    event_queue.push(code);
}