#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <strings.h>

#include "instruction.h"
#include "printRoutines.h"

#define ERROR_RETURN -1
#define SUCCESS 0

#define MAX_LINE 256

static void addBreakpoint(uint64_t address);
static void deleteBreakpoint(uint64_t address);
static void deleteAllBreakpoints(void);
static int  hasBreakpoint(uint64_t address);

struct Node *head = NULL;

struct Node
{
  uint64_t data;
  struct Node *next;
};

int main(int argc, char **argv)
{

  int fd;
  struct stat st;

  machine_state_t state;
  y86_instruction_t nextInstruction;
  memset(&state, 0, sizeof(state));

  char line[MAX_LINE + 1], previousLine[MAX_LINE + 1] = "";
  char *command, *parameters;
  int c;

  // Verify that the command line has an appropriate number of
  // arguments
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s InputFilename [startingPC]\n", argv[0]);
    return ERROR_RETURN;
  }

  // First argument is the file to read, attempt to open it for
  // reading and verify that the open did occur.
  fd = open(argv[1], O_RDONLY);

  if (fd < 0) {
    fprintf(stderr, "Failed to open %s: %s\n", argv[1], strerror(errno));
    return ERROR_RETURN;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(stderr, "Failed to stat %s: %s\n", argv[1], strerror(errno));
    close(fd);
    return ERROR_RETURN;
  }

  state.programSize = st.st_size;

  // If there is a 2nd argument present it is an offset so convert it
  // to a numeric value.
  if (3 <= argc) {
    errno = 0;
    state.programCounter = strtoul(argv[2], NULL, 0);
    if (errno != 0) {
      perror("Invalid program counter on command line");
      close(fd);
      return ERROR_RETURN;
    }
    if (state.programCounter > state.programSize) {
      fprintf(stderr, "Program counter on command line (%lu) "
	      "larger than file size (%lu).\n",
	      state.programCounter, state.programSize);
      close(fd);
      return ERROR_RETURN;
    }
  }

  // Maps the entire file to memory. This is equivalent to reading the
  // entire file using functions like fread, but the data is only
  // retrieved on demand, i.e., when the specific region of the file
  // is needed.
  state.programMap = mmap(NULL, state.programSize, PROT_READ | PROT_WRITE,
			  MAP_PRIVATE, fd, 0);
  if (state.programMap == MAP_FAILED) {
    fprintf(stderr, "Failed to map %s: %s\n", argv[1], strerror(errno));
    close(fd);
    return ERROR_RETURN;
  }

  // Move to first non-zero byte
  while (!state.programMap[state.programCounter]) state.programCounter++;

  printf("# Opened %s, starting PC 0x%lX\n", argv[1], state.programCounter);

  fetchInstruction(&state, &nextInstruction);
  printInstruction(stdout, &nextInstruction);

  while(1) {

    // Show prompt, but only if input comes from a terminal
    if (isatty(STDIN_FILENO))
      printf("> ");

    // Read one line, if EOF break loop
    if (!fgets(line, sizeof(line), stdin))
      break;

    // If line could not be read entirely
    if (!strchr(line, '\n')) {
      // Read to the end of the line
      while ((c = fgetc(stdin)) != EOF && c != '\n');
      if (c == '\n') {
	printErrorCommandTooLong(stdout);
	continue;
      }
      else {
	// In this case there is an EOF at the end of a line.
	// Process line as usual.
      }
    }

    // Obtain the command name, separate it from the arguments.
    command = strtok(line, " \t\n\f\r\v");
    // If line is blank, repeat previous command.
    if (!command) {
      strcpy(line, previousLine);
      command = strtok(line, " \t\n\f\r\v");
      // If there's no previous line, do nothing.
      if (!command) continue;
    }

    // Get the arguments to the command, if provided.
    parameters = strtok(NULL, "\n\r");

    sprintf(previousLine, "%s %s\n", command, parameters ? parameters : "");

    if (strcasecmp(command, "QUIT") == 0 || strcasecmp(command, "EXIT") == 0)
    {
      break;
    }
    else if (strcasecmp(command, "STEP") == 0)
    {
      // If the instruction is halt, the program counter remains unmodified.
      // If the instruction is invalid, an error message must be printed
      //  and the program counter remains unmodified.
      if (executeInstruction(&state, &nextInstruction) == 0)
      {
        printInstruction(stdout, &nextInstruction);
      }
      else
      {
        fetchInstruction(&state, &nextInstruction);
        printInstruction(stdout, &nextInstruction);
      }
    }
    else if (strcasecmp(command, "RUN") == 0)
    {
      int validInstruction;
      validInstruction = executeInstruction(&state, &nextInstruction);
      if (validInstruction == 0) // invalid instruction
      {
        printInstruction(stdout, &nextInstruction);
        continue;
      }
      else
      {
        fetchInstruction(&state, &nextInstruction);
      }

      // keep running rest of the instructions
      while (1)
      {
        if ((nextInstruction.icode == I_HALT && nextInstruction.ifun == 0) ||
            hasBreakpoint(state.programCounter))
        {
          printInstruction(stdout, &nextInstruction);
          break;
        }

        validInstruction = executeInstruction(&state, &nextInstruction);
        if (validInstruction == 0) // invalid instruction
        {
          printInstruction(stdout, &nextInstruction);
          break;
        }
        else // valid case
        {
          fetchInstruction(&state, &nextInstruction);
        }
      }
    }
    else if (strcasecmp(command, "NEXT") == 0)
    {
      if (nextInstruction.icode == I_CALL)
      {
        uint64_t saveRegister = state.registerFile[4];

        // Inside the CALL method
        while ((nextInstruction.icode != I_HALT) && (!hasBreakpoint(state.programCounter)))
        {
          if (executeInstruction(&state, &nextInstruction) == 1)
          { //if successful execution, continue
            fetchInstruction(&state, &nextInstruction);
            if (saveRegister == state.registerFile[4])
            {
              printInstruction(stdout, &nextInstruction);
              break;
            }
          }
          else
          {
            //if not successful execution, print error
            printInstruction(stdout, &nextInstruction);
            break;
          }
        }
      }
      else
      {
        //if not call function, same as STEP command
        if (nextInstruction.icode == I_HALT && nextInstruction.ifun == 0)
        {
          printInstruction(stdout, &nextInstruction);
          continue;
        }
        else
        {
          //if successful execution, go to next instruction
          if (executeInstruction(&state, &nextInstruction) == 1)
          {
            fetchInstruction(&state, &nextInstruction);
            printInstruction(stdout, &nextInstruction);
          }
          else
          {
            //if not successful execution, print error
            printInstruction(stdout, &nextInstruction);
          }
        }
      }
    }
    else if (strcasecmp(command, "JUMP") == 0)
    {
      // parameter is NULL case:
      if(!parameters){
        printErrorInvalidCommand(stdout, command, parameters);
        continue;
      }

      uint64_t address = strtoul(parameters, NULL, 16);

      state.programCounter = address;
      fetchInstruction(&state, &nextInstruction);
      printInstruction(stdout, &nextInstruction);
    }
    else if (strcasecmp(command, "BREAK") == 0)
    {
      if (!parameters)
      {
        printErrorInvalidCommand(stdout, command, parameters);
        continue;
      }

      uint64_t address = strtoul(parameters, NULL, 16);
      addBreakpoint(address);
    }
    else if (strcasecmp(command, "DELETE") == 0)
    {
      if (!parameters)
      {
        printErrorInvalidCommand(stdout, command, parameters);
        continue;
      }
      uint64_t address = strtoul(parameters, NULL, 16);
      deleteBreakpoint(address);
    }
    else if (strcasecmp(command, "REGISTERS") == 0)
    {
      for (int i = R_RAX; i <= R_R14; ++i)
      {
        printRegisterValue(stdout, &state, i);
      }
    }
    else if (strcasecmp(command, "EXAMINE") == 0)
    {
      if(!parameters){
        printErrorInvalidCommand(stdout, command, parameters);
        continue;
      }

      uint64_t address = strtoul(parameters, NULL, 16);
      printMemoryValueQuad(stdout, &state, address);
    }
    else
    {
      //Any command not listed above should be rejected with an error message
      printErrorInvalidCommand(stdout, command, parameters);
    }
  }

  deleteAllBreakpoints();
  munmap(state.programMap, state.programSize);
  close(fd);
  return SUCCESS;
}

/* Adds an address to the list of breakpoints. If the address is
 * already in the list, it is not added again. */
static void addBreakpoint(uint64_t address) {

  struct Node *temp = head, *prev;

  // Search for address to be added already in list
  if (temp != NULL && temp->data == address)
  {
    return;
  }
  while (temp != NULL && temp->data != address)
  {
    prev = temp;
    temp = temp->next;
  }

  // If address is not in the list, add breakpoint to list
  if (temp == NULL)
  {
    struct Node *temp = (struct Node *)malloc(sizeof(struct Node));
    temp->data = address;
    temp->next = (head);
    (head) = temp;
  }

  free(temp);
}

/* Deletes an address from the list of breakpoints. If the address is
 * not in the list, nothing happens. */
static void deleteBreakpoint(uint64_t address) {

  struct Node *temp = head, *prev;

  if (temp != NULL && temp->data == address)
  {
    head = temp->next;
    free(temp);
    return;
  }

  // Search for address to be deleted
  while (temp != NULL && temp->data != address)
  {
    prev = temp;
    temp = temp->next;
  }

  // If address is not in the list, nothing happens
  if (temp == NULL)
    return;

  // temp != NULL, address to be deleted is found
  prev->next = temp->next;

  free(temp);
}

/* Deletes and frees all breakpoints. */
static void deleteAllBreakpoints(void) {

  struct Node *next;

  while (head != NULL)
  {
    next = head->next;
    free(head);
    head = next;
  }
}

/* Returns true (non-zero) if the address corresponds to a breakpoint
 * in the list of breakpoints, or false (zero) otherwise. */
static int hasBreakpoint(uint64_t address) {

  struct Node *current = head, *prev;

  while (current != NULL)
  {
    if (current->data == address)
    {
      return 1;
    }
    prev = current;
    current = current->next;
  }
  return 0;
}
