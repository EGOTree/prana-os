#ifndef SHELL_IA938IEU
#define SHELL_IA938IEU

//shell attributes
#define SHELL_HEADER 1
#define SHELL_FOOTER 2

#define SHELL_MAX_ARGC      10

int shell_init(char *prmpt);
void shell_kbrd_cb(char c);

#endif 