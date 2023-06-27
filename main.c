#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
  Function Declarations for builtin shell commands:
 */
int myshell_cd(char **inp_args);
int myshell_help(char **inp_args);
int myshell_exit(char **inp_args);
int myshell_echo(char **inp_args);
int print_his(char **inp_args);
int free_his(char **inp_args);

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
    "cd",
    "help",
    "exit",
    "myshell_echo",
    "print_his",
    "free_his"};

int (*builtin_func[])(char **) = {
    &myshell_cd,
    &myshell_help,
    &myshell_exit,
    &myshell_echo,
    &print_his,
    &free_his};

int myshell_num_builtins()
{
    return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/

/**
   @brief Builtin command: change directory.
   @param args List of args.  args[0] is "cd".  args[1] is the directory.
   @return Always returns 1, to continue executing.
 */
int myshell_cd(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "myshell: expected argument to \"cd\"\n");
    }
    else
    {
        if (chdir(args[1]) != 0)
        {
            perror("myshell");
        }
    }
    return 1;
}

/**
   @brief Builtin command: print help.
   @param args List of args.  Not examined.
   @return Always returns 1, to continue executing.
 */
int myshell_help(char **inp_args)
{
    int i;
    printf("Type program names and arguments, and hit enter.\n");
    printf("The following are built in:\n");

    for (i = 0; i < myshell_num_builtins(); i++)
    {
        printf("  %s\n", builtin_str[i]);
    }

    printf("Use the main command for information on other programs.\n");
    return 1;
}

/**
   @brief Builtin command: exit.
   @param inp_args List of args.  Not examined.
   @return Always returns 0, to terminate execution.
 */
int myshell_exit(char **inp_args)
{
    return 0;
}

int myshell_echo(char **inp_args){
  // printf("\n Enter input: "); 
  char str[50];

  printf("\n Enter input: ");
  scanf("%[^\n]+", str);

  printf(" Echo : %s \n" , str);

  return 1;
}

struct History {
  char** inp_args;
  int max_size; 
  int start; // points to last empty place in inp_args
};

struct History* history; 


void initialize_history(struct History* history, int Size) {
  history->max_size = Size;
  history->inp_args = malloc(Size * sizeof(char*));
  int i;
  for (i = 0; i < Size; ++i) {
    history->inp_args[i] = NULL;
  }
}

void add_to_history(struct History* history, char* commandline) {
    // checked if that place is empty
  if (history->inp_args[history->start] != NULL) {
    free(history->inp_args[history->start]);
  }
//   added history 
  history->inp_args[history->start] = strdup(commandline);
  history->start = (history->start + 1) % history->max_size;
}

void print_history(struct History* history) {
  int begin = history->start;
  int Size = history -> max_size;
  for (int i = 0; i < Size; ++i) {
    if (history->inp_args[begin] != NULL) {
      printf("%s\n", history->inp_args[begin]);
    }
    begin--;
    begin += history -> max_size;
    begin %= history -> max_size;
  }
}

void free_history(struct History* history) {
    int Size = history -> max_size;
  for (int i = 0; i < Size; ++i) {
    if (history->inp_args[i] != NULL) {
      free(history->inp_args[i]);
    }
  }
  free(history->inp_args);
}

int print_his(char **inp_args){
  print_history(history);
}

int free_his(char **inp_args){
  free_history(history);
  return 1;
}


/**
  @brief Launch a program and wait for it to terminate.
  @param inp_args Null terminated list of arguments (including program).
  @return Always returns 1, to continue execution.
 */
int myshell_launch(char **inp_args)
{
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0)
    {
        // Child process
        if (execvp(inp_args[0], inp_args) == -1)
        {
            perror("myshell");
        }
        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
        // Error forking
        perror("myshell");
    }
    else
    {
        // Parent process
        do
        {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

/**
   @brief Execute shell built-in or launch program.
   @param inp_args Null terminated list of arguments.
   @return 1 if the shell should continue running, 0 if it should terminate
 */
int myshell_execute(char **inp_args)
{
    int i;

    if (inp_args[0] == NULL)
    {
        // An empty command was entered.
        return 1;
    }

    for (i = 0; i < myshell_num_builtins(); i++)
    {
        if (strcmp(inp_args[0], builtin_str[i]) == 0)
        {
            return (*builtin_func[i])(inp_args);
        }
    }

    return myshell_launch(inp_args);
}

/**
   @brief Read a line of input from stdin.
   @return The line from stdin.
 */
char *myshell_read_inp(void)
{
    char *inp_line = NULL;
    ssize_t bufsize = 0;
    if (getline(&inp_line, &bufsize, stdin) == -1)
    {
        if (feof(stdin))
        {
            // reached end of line
            exit(EXIT_SUCCESS);
        }
        else
        {
            perror("shm: getline\n");
            exit(EXIT_FAILURE);
        }
    }
    return inp_line;
}

#define BUFSIZE 64
#define DELIM " \t\r\n\a"
/**
   @brief parsing inputs (very naively).
   @param inp_line input line.
   @return Null-terminated array of inputs.
 */

char **myshell_parseinp(char *inp_line)
{
    int bufsize = BUFSIZE;
    int curr_pos = 0;
    char **inp_args = malloc(bufsize * sizeof(char *));
    char *inp_arg;

    if (!inp_args)
    {
        fprintf(stderr, "myshell: allocation error");
        exit(EXIT_FAILURE);
    }

    inp_arg = strtok(inp_line, DELIM);
    while (inp_arg != NULL)
    {
        inp_args[curr_pos] = inp_arg;
        curr_pos++;

        if (curr_pos >= bufsize)
        {
            bufsize += BUFSIZE;
            inp_args = realloc(inp_args, bufsize * sizeof(char *));
            if (!inp_args)
            {
                fprintf(stderr, "myshell, allocation error");
                exit(EXIT_FAILURE);
            }
        }

        inp_arg = strtok(NULL, DELIM);
    }
    inp_args[curr_pos] = NULL;
    return inp_args;
}

/**
 @brief loop getting input and executing it
*/

void myshell_loop()
{
    char *inp_line;
    char **inp_args;
    int inp_status;

    history = (struct History*)malloc(sizeof(struct History));
    initialize_history(history, 64); 
    // here i used size as 1024 so at max 1024 commands can be stored as history at any instant  


    do
    {
        printf("> ");
        inp_line = myshell_read_inp();
        inp_args = myshell_parseinp(inp_line);
        inp_status = myshell_execute(inp_args);


        add_to_history(history, inp_line);

        free(inp_line);
        free(inp_args);
    } while (inp_status);

}

/**
 @brief Mainf function
 @param argc Argument count
 @param argv Argument vector
 @return statuc code

*/

int main(int argc, char const *argv[])
{
    // config files

    myshell_loop();

    // termination
    return 0;
}

