#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/**
 * @brief used to ignore the Ctrl-C user input
 */
void ignore() {}

/**
 * @brief used for disambiguation between command types
 */
typedef enum cmd_type
{
  EXTERNAL,
  SETENV,
  UNSETENV,
  CD,
  EXIT
} cmd_type;

/**
 * @brief strings for matching commands above
 */
const char *SETENV_COMMAND = "setenv";
const char *UNSETENV_COMMAND = "unsetenv";
const char *CD_COMMAND = "cd";
const char *EXIT_COMMAND = "exit";

/**
 * @brief tokenize text command by spaces and newlines
 * 
 * @param command line of text
 * @return char** tokens from command
 * Static max number of tokens.
 */
#define TOKENS_SIZE 64
char **tokenize(char *command)
{
  // Allocate starting space.
  char **tokens = malloc(sizeof(char *) * TOKENS_SIZE);
  memset(tokens, 0, sizeof(char *) * TOKENS_SIZE);

  // Get first token
  char *token = strtok(command, " \n");

  // Loop over all tokens
  int token_i = 0;
  while (token != NULL)
  {
    int token_length = strlen(token);
    tokens[token_i] = malloc(sizeof(char) * (token_length + 1));
    tokens[token_i][token_length] = '\0';
    memcpy(tokens[token_i], token, token_length);
    ++token_i;
    token = strtok(NULL, " \n");
  }

  return tokens;
}

cmd_type check_command(char *command)
{
  if (strncmp(CD_COMMAND, command, 3) == 0)
    return CD;
  if (strncmp(EXIT_COMMAND, command, 5) == 0)
    return EXIT;
  if (strncmp(SETENV_COMMAND, command, 7) == 0)
    return SETENV;
  if (strncmp(UNSETENV_COMMAND, command, 9) == 0)
    return UNSETENV;
  return EXTERNAL;
}

#define LINE_BUFFER_SIZE 1024
char **get_line(FILE *input)
{
  char line_buffer[1024];

  // Read into buffer.
  if (fgets(line_buffer, LINE_BUFFER_SIZE, input) == NULL)
    return NULL;

  // Tokenize line
  char **tokens = tokenize(line_buffer);

  // Check if the line is empty
  if (tokens[0] == NULL)
  {
    free(tokens);
    return NULL;
  }
  return tokens;
}

/**
 * @brief run the command with its args in tokens
 * 
 * @param tokens command followed by args
 * @return int
 *  1 for normal
 *  0 for exit
 *  < 0 for failure
 */
int run_command(char **tokens)
{
  int ret_val = 1;

  // Is this one of our commands?
  cmd_type cmd = check_command(tokens[0]);

  switch (cmd)
  {
  case SETENV:
    if ((tokens[1] == NULL) || (tokens[2] == NULL))
    {
      printf("Expected usage: setenv <variable> <value>\n");
      break;
    }
    setenv(tokens[1], tokens[2], 1);
    break;
  case UNSETENV:
    if (tokens[1] == NULL)
    {
      printf("Expected usage: unsetenv <variable>\n");
      break;
    }
    unsetenv(tokens[1]);
    break;
  case CD:
    if (tokens[1] == NULL)
    {
      printf("Expected usage: cd <path>\n");
      break;
    }
    chdir(tokens[1]);
    break;
  case EXIT:
    ret_val = 0;
    break;
  case EXTERNAL:
  default:
  {
    pid_t cid = fork();
    if (cid == 0)
    {
      execvp(tokens[0], tokens);
      fprintf(stderr, "Error creating new process.\n");
      ret_val = -1;
    }
    if (cid < 0)
    {
      fprintf(stderr, "Error forking.\n");
      ret_val = -1;
    }
    waitpid(cid, NULL, 0);
  }
  break;
  }

  return ret_val;
}

/**
 * @brief cleans up past set of tokens
 * 
 * @param tokens 
 */
void cleanup(char **tokens)
{
  char **curr = tokens;
  while (*curr != NULL)
  {
    free(*curr);
    ++curr;
  }
  if (tokens != NULL)
    free(tokens);
}

/**
 * @brief this sets up the application, and takes care of setup
 * 
 * @return int 
 */
int initalize()
{
  // Remap SIGINT to the 'ignore it' function.
  signal(SIGINT, ignore);

  // Get username
  char *username = getlogin();
  size_t username_size = strlen(username);

  // Create path to rc file with username
  size_t kapishrc_path_size = username_size + 17;
  char kapishrc_path[kapishrc_path_size];
  sprintf(kapishrc_path, "/home/%s/.kapishrc", username);

  // Read in the rc file
  FILE *kapishrc = fopen(kapishrc_path, "r");
  if (kapishrc == NULL)
    return 0;

  while (1)
  {
    char **tokens = get_line(kapishrc);
    if (tokens == NULL)
      break;

    run_command(tokens);
    cleanup(tokens);
  }

  fclose(kapishrc);

  return 0;
}

/**
 * @brief main loop of kapish
 * 
 * This reads a line of user input, and allows it to
 * be passed to an internal function or to execute the
 * given external program.
 * 
 * @return int 
 */
int loop()
{
  /*
   * program_status
   *    > 0 : everything's good.
   *    = 0 : ready to exit.
   *    < 0 : error occured, exit.
   */
  int program_status = 1;

  while (program_status > 0)
  {
    // Print prompt
    printf("? ");

    // Get and process current command
    char **tokens = get_line(stdin);
    if (tokens == NULL)
      continue;

    program_status = run_command(tokens);
    cleanup(tokens);
  }
  return program_status;
}

int main(int argc, char **argv)
{
  int ret_val = initalize();
  if (ret_val != 0)
  {
    exit(ret_val);
  }

  ret_val = loop();
  if (ret_val != 0)
  {
    exit(ret_val);
  }
}