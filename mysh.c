// Shell.

#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

// four types cmd
#define CMD_EXEC  1
#define CMD_REDIRECT 2
#define CMD_PIPELINE  3
#define CMD_LIST  4
#define CMD_MAX_ARGS 10

struct cmd {
    int cmd_code;
};
struct exec_cmd {
    int cmd_code;
    char *argv[CMD_MAX_ARGS];
    char *end_argv[CMD_MAX_ARGS];
};
struct redirect_cmd {
    int cmd_code;
    struct cmd *cmd;
    char *file;
    char *efile;
    int mode;
    int fd;
};
struct pipeline_cmd {
    int cmd_code;
    struct cmd *left_part;
    struct cmd *right_part;
};
struct list_cmd {
    int cmd_code;
    struct cmd *left_part;
    struct cmd *right_part;
};
void my_exit(char *s)
{
    fprintf(2, "mysh exit: %s\n", s);
    exit(1);
}
int my_fork(void)
{
    int pid;
    pid = fork();
    if(pid == -1)
        my_exit("process fork error");
    return pid;
}

struct cmd* exec_cmd(void)
{
    struct exec_cmd *cmd;

    cmd = malloc(sizeof(*cmd));
    memset(cmd, 0, sizeof(*cmd));
    cmd->cmd_code = CMD_EXEC;
    return (struct cmd*)cmd;
}

struct cmd* redirect_cmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
{
    struct redirect_cmd *cmd;

    cmd = malloc(sizeof(*cmd));
    memset(cmd, 0, sizeof(*cmd));
    cmd->cmd_code = CMD_REDIRECT;
    cmd->cmd = subcmd;
    cmd->file = file;
    cmd->efile = efile;
    cmd->mode = mode;
    cmd->fd = fd;
    return (struct cmd*)cmd;
}

struct cmd* pipeline_cmd(struct cmd *left_part, struct cmd *right_part)
{
    struct pipeline_cmd *cmd;

    cmd = malloc(sizeof(*cmd));
    memset(cmd, 0, sizeof(*cmd));
    cmd->cmd_code = CMD_PIPELINE;
    cmd->left_part = left_part;
    cmd->right_part = right_part;
    return (struct cmd*)cmd;
}

struct cmd*
list_cmd(struct cmd *left_part, struct cmd *right_part)
{
    struct list_cmd *cmd;

    cmd = malloc(sizeof(*cmd));
    memset(cmd, 0, sizeof(*cmd));
    cmd->cmd_code = CMD_LIST;
    cmd->left_part = left_part;
    cmd->right_part = right_part;
    return (struct cmd*)cmd;
}


char cmd_splits[] = " \t\r\n\v";
char symbols[] = "<|>;";

int
gettoken(char **ps, char *es, char **q, char **eq)
{
    char *s;
    int ret;

    s = *ps;
    while(s < es && strchr(cmd_splits, *s))
        s++;
    if(q)
        *q = s;
    ret = *s;
    switch(*s){
        case 0:
            break;
        case '|':
        case ';':
        case '<':
            s++;
            break;
        case '>':
            s++;
            if(*s == '>'){
                ret = '+';
                s++;
            }
            break;
        default:
            ret = 'a';
            while(s < es && !strchr(cmd_splits, *s) && !strchr(symbols, *s))
                s++;
            break;
    }
    if(eq)
        *eq = s;

    while(s < es && strchr(cmd_splits, *s))
        s++;
    *ps = s;
    return ret;
}

int
peek(char **ps, char *es, char *toks)
{
    char *s;

    s = *ps;
    while(s < es && strchr(cmd_splits, *s))
        s++;
    *ps = s;
    return *s && strchr(toks, *s);
}

struct cmd* add_cmd_terminate(struct cmd *cmd)
{
    int i;
    struct exec_cmd *e_cmd;
    struct list_cmd *l_cmd;
    struct pipeline_cmd *p_cmd;
    struct redirect_cmd *r_cmd;

    if(cmd == 0)
        return 0;

    switch(cmd->cmd_code){
        case CMD_EXEC:
            e_cmd = (struct exec_cmd*)cmd;
            for(i=0; e_cmd->argv[i]; i++)
                *e_cmd->end_argv[i] = 0;
            break;

        case CMD_REDIRECT:
            r_cmd = (struct redirect_cmd*)cmd;
            add_cmd_terminate(r_cmd->cmd);
            *r_cmd->efile = 0;
            break;

        case CMD_PIPELINE:
            p_cmd = (struct pipeline_cmd*)cmd;
            add_cmd_terminate(p_cmd->left_part);
            add_cmd_terminate(p_cmd->right_part);
            break;

        case CMD_LIST:
            l_cmd = (struct list_cmd*)cmd;
            add_cmd_terminate(l_cmd->left_part);
            add_cmd_terminate(l_cmd->right_part);
            break;
    }
    return cmd;
}
struct cmd *process_line_cmd(char**, char*);
struct cmd *process_pipeline_cmd(char**, char*);
struct cmd *process_exec_cmd(char**, char*);
struct cmd *add_cmd_terminate(struct cmd*);
// Execute cmd.  Never returns.
void start_cmd(struct cmd *cmd)
{
    int p[2];
    struct exec_cmd *e_cmd;
    struct list_cmd *l_cmd;
    struct pipeline_cmd *p_cmd;
    struct redirect_cmd *r_cmd;

    if(cmd == 0)
        exit(1);

    switch(cmd->cmd_code){
        default:
            my_exit("start_cmd");

        case CMD_EXEC:
            e_cmd = (struct exec_cmd*)cmd;
            if(e_cmd->argv[0] == 0)
                exit(1);
            exec(e_cmd->argv[0], e_cmd->argv);
            fprintf(2, "exec %s failed\n", e_cmd->argv[0]);
            break;

        case CMD_REDIRECT:
            r_cmd = (struct redirect_cmd*)cmd;
            close(r_cmd->fd);
            if(open(r_cmd->file, r_cmd->mode) < 0){
                fprintf(2, "open %s failed\n", r_cmd->file);
                exit(1);
            }
            start_cmd(r_cmd->cmd);
            break;

        case CMD_LIST:
            l_cmd = (struct list_cmd*)cmd;
            if(my_fork() == 0)
                start_cmd(l_cmd->left_part);
            wait(0);
            start_cmd(l_cmd->right_part);
            break;

        case CMD_PIPELINE:
            p_cmd = (struct pipeline_cmd*)cmd;
            if(pipe(p) < 0)
                my_exit("pipe");
            if(my_fork() == 0){
                close(1);
                dup(p[1]);
                close(p[0]);
                close(p[1]);
                start_cmd(p_cmd->left_part);
            }
            if(my_fork() == 0){
                close(0);
                dup(p[0]);
                close(p[0]);
                close(p[1]);
                start_cmd(p_cmd->right_part);
            }
            close(p[0]);
            close(p[1]);
            wait(0);
            wait(0);
            break;
    }
    exit(0);
}

int
get_input_cmd(char *buf, int nbuf)
{
    fprintf(2, ">>> ");
    memset(buf, 0, nbuf);
    gets(buf, nbuf);
    if(buf[0] == 0) // EOF
        return -1;
    return 0;
}

struct cmd* process_pipeline_cmd(char **ps, char *es)
{
    struct cmd *cmd;

    cmd = process_exec_cmd(ps, es);
    if(peek(ps, es, "|")){
        gettoken(ps, es, 0, 0);
        cmd = pipeline_cmd(cmd, process_pipeline_cmd(ps, es));
    }
    return cmd;
}

struct cmd* process_redirect_cmd(struct cmd *cmd, char **ps, char *es)
{
    int tok;
    char *q, *eq;

    while(peek(ps, es, "<>")){
        tok = gettoken(ps, es, 0, 0);
        if(gettoken(ps, es, &q, &eq) != 'a')
            my_exit("missing file for redirection");
        switch(tok){
            case '<':
                cmd = redirect_cmd(cmd, q, eq, O_RDONLY, 0);
                break;
            case '>':
                cmd = redirect_cmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
                break;
            case '+':  // >> append mode for redirect
                cmd = redirect_cmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
                break;
        }
    }
    return cmd;
}

struct cmd* process_exec_cmd(char **ps, char *es)
{
    char *q, *eq;
    int tok, argc;
    struct exec_cmd *cmd;
    struct cmd *ret;

    ret = exec_cmd();
    cmd = (struct exec_cmd*)ret;

    argc = 0;
    ret = process_redirect_cmd(ret, ps, es);
    while(!peek(ps, es, "|;")){
        if((tok=gettoken(ps, es, &q, &eq)) == 0)
            break;
        if(tok != 'a')
            my_exit("syntax");
        cmd->argv[argc] = q;
        cmd->end_argv[argc] = eq;
        argc++;
        if(argc >= CMD_MAX_ARGS)
            my_exit("too many args");
        ret = process_redirect_cmd(ret, ps, es);
    }
    cmd->argv[argc] = 0;
    cmd->end_argv[argc] = 0;
    return ret;
}

struct cmd* process_line_cmd(char **ps, char *es)
{
    struct cmd *cmd;

    cmd = process_pipeline_cmd(ps, es);
    if(peek(ps, es, ";")){
        gettoken(ps, es, 0, 0);
        cmd = list_cmd(cmd, process_line_cmd(ps, es));
    }
    return cmd;
}

struct cmd* process_cmd(char *s)
{
    char *es;
    struct cmd *cmd;

    es = s + strlen(s);
    cmd = process_line_cmd(&s, es);
    peek(&s, es, "");
    if(s != es){
        fprintf(2, "left_partovers: %s\n", s);
        my_exit("syntax");
    }
    add_cmd_terminate(cmd);
    return cmd;
}

int main(void)
{
    static char input[256];
    fprintf(1, "==================          ****Welcome to my shell****     =====================================\n");
    fprintf(1, "                   CTRL+D+ENTER or input command quit/exit to quit                               \n");
    fprintf(1, "=================================================================================================\n");
    while(get_input_cmd(input, sizeof(input)) >= 0){
        if(input[0] == 'c' && input[1] == 'd' && input[2] == ' '){
            // Chdir must be called by the parent, not the child.
            input[strlen(input)-1] = 0;  // chop \n
            if(chdir(input+3) < 0)
                fprintf(2, "cannot cd %s\n", input+3);
            continue;
        }
        if(input[0] == 'e' && input[1] == 'x' && input[2] == 'i' &&input[3] == 't'){
            fprintf(1, "Thank you for trying my shell\n");
            exit(0);
        }
        if(input[0] == 'q' && input[1] == 'u' && input[2] == 'i' &&input[3] == 't'){
            fprintf(1, "Thank you for trying my shell\n");
            exit(0);
        }
        if (my_fork() == 0)// process the command in child process
        {
            struct cmd *my_cmd;
            my_cmd = process_cmd(input);
            start_cmd(my_cmd);
        }
        wait(0);//parent process waiting here for child complete
    }
    exit(0);
    return 0;
}




