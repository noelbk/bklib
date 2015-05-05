/* Use ShellExecuteEx on Win32 and return bogus pid, or call proc_start on unix */
procid_t
proc_shellex(char *argv0, char *cmdline, char *env) {
    return proc_start(argv0, cmdline, env);
}


