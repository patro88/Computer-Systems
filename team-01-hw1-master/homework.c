/*
 * file:        homework.c
 * description: Skeleton for homework 1
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, Jan. 2012
 * $Id: homework.c 500 2012-01-15 16:15:23Z pjd $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "elf32.h"
#include "uprog.h"

/***********************************/
/* Declarations for code in misc.c */
/***********************************/

typedef int *stack_ptr_t;
extern void init_memory(void);
extern void do_switch(stack_ptr_t *location_for_old_sp, stack_ptr_t new_value);
extern stack_ptr_t setup_stack(int *stack, void *func);
extern int get_term_input(char *buf, size_t len);
extern void init_terms(void);

extern void  *proc1;
extern void  *proc1_stack;
extern void  *proc2;
extern void  *proc2_stack;
extern void  **vector;

stack_ptr_t stack_q3p1 = NULL;
stack_ptr_t stack_q3p2 = NULL;
stack_ptr_t stack_main = NULL;

/***********************************************/
/********* Your code starts here ***************/
/***********************************************/

/*
 * Factoring for reading ELF file
 */
int read_elf(char* fname, void* proc)
{
   struct elf32_ehdr hdr;
   int fd = open(fname, O_RDONLY);
   if(fd == -1)
        return 0;
   read(fd, &hdr, sizeof(hdr));

   int n = hdr.e_phnum;
   struct elf32_phdr phdrs[n];
   lseek(fd, hdr.e_phoff, SEEK_SET);
   read(fd, &phdrs, sizeof(phdrs));

   int i;
   for(i=0; i<hdr.e_phnum; i++)
      if(phdrs[i].p_type == PT_LOAD)
      {
         lseek(fd, phdrs[i].p_offset, SEEK_SET);
         read(fd, proc, phdrs[i].p_filesz);
         break;
      }
     
   close(fd); 
   return 1;
}

/*
 * Question 1.
 *
 * The micro-program q1prog.c has already been written, and uses the
 * 'print' micro-system-call (index 0 in the vector table) to print
 * out "Hello world".
 *
 * You'll need to write the (very simple) print() function below, and
 * then put a pointer to it in vector[0].
 *
 * Then you read the micro-program 'q1prog' into memory starting at
 * address 'proc1', and execute it, printing "Hello world".
 *
 */
void print(char *line)
{
   printf("%s",line);
}

void q1(void)
{
   vector[0] = print;
   char* fname = "q1prog";
   int status = read_elf(fname, proc1);
   if(status ==1)
   {
      void(*progq1)(void) = proc1;
      progq1();
   }
   else
      printf("Error: Could not read ELF header\n");
}


/*
 * Question 2.
 *
 * Add two more functions to the vector table:
 *   void readline(char *buf, int len) - read a line of input into 'buf'
 *   char *getarg(int i) - gets the i'th argument (see below)

 * Write a simple command line which prints a prompt and reads command
 * lines of the form 'cmd arg1 arg2 ...'. For each command line:
 *   - save arg1, arg2, ... in a location where they can be retrieved
 *     by 'getarg'
 *   - load and run the micro-program named by 'cmd'
 *   - if the command is "quit", then exit rather than running anything
 *
 * Note that this should be a general command line, allowing you to
 * execute arbitrary commands that you may not have written yet. You
 * are provided with a command that will work with this - 'q2prog',
 * which is a simple version of the 'grep' command.
 *
 * NOTE - your vector assignments have to mirror the ones in vector.s:
 *   0 = print
 *   1 = readline
 *   2 = getarg
 */
void readline(char *buf, int len) /* vector index = 1 */
{
    fgets(buf, len, stdin);
}

char argv[10][20];
char linebuf[100];

char *getarg(int i)		/* vector index = 2 */
{ 
    if(argv[i+1][0] == '\000')
  	return 0;
    else
	return argv[i+1];
}

char *strwrd(char *s, char *buf, size_t len, char *delim)
{
    s += strspn(s, delim);
    int n = strcspn(s, delim);  /* count the span (spn) of bytes in */
    if (len-1 < n)              /* the complement (c) of *delim */
        n = len-1;
    memcpy(buf, s, n);
    buf[n] = 0;
    s += n;
    return (*s == 0) ? NULL : s;
}
/*
 * Note - see c-programming.pdf for sample code to split a line into
 * separate tokens. 
 */

void q2(void)
{
    vector[0] = print;
    vector[1] = readline;
    vector[2] = getarg;
 
    while (1) 
    {
	/* get a line */
	readline(linebuf, sizeof(linebuf));

	/* if zero words, continue */
	
        /* split it into words */
        int argc = 0;
        char *line = linebuf;
        for (argc = 0; argc < 10; argc++) 
	{
	    line = strwrd(line, argv[argc], sizeof(argv[argc]), " \t\n");

	    if (line == NULL)
	        break;
        }	
	
	/* if first word is "quit", break */
	if(strcmp(argv[0],"quit") == 0)
	    break;
	else
	{
	    /* load and run the command */
	    char* fname = argv[0];
    	    int status = read_elf(fname, proc1);
	    if(status == 1)
	    {
	        void(*progq2)(void) = proc1;
	        progq2();
	    }
	    else
	        printf("Error: Could not find command\n");
	}
    }
}

/*
 * Question 3.
 *
 * Create two processes which switch back and forth.
 *
 * You will need to add another 3 functions to the table:
 *   void yield12(void) - save process 1, switch to process 2
 *   void yield21(void) - save process 2, switch to process 1
 *   void uexit(void) - return to original homework.c stack
 *
 * The code for this question will load 2 micro-programs, q3prog1 and
 * q3prog2, which are provided and merely consists of interleaved
 * calls to yield12() or yield21() and print(), finishing with uexit().
 *
 * Hints:
 * - Use setup_stack() to set up the stack for each process. It returns
 *   a stack pointer value which you can switch to.
 * - you need a global variable for each process to store its context
 *   (i.e. stack pointer)
 * - To start you use do_switch() to switch to the stack pointer for 
 *   process 1
 */

void yield12(void)		/* vector index = 3 */
{
   do_switch(&stack_q3p1, stack_q3p2);
}

void yield21(void)		/* vector index = 4 */
{
   do_switch(&stack_q3p2, stack_q3p1);
}

void uexit(void)		/* vector index = 5 */
{
   do_switch(&stack_q3p2, stack_main);
}

void q3(void)
{
   vector[0] = print;
   vector[1] = readline;
   vector[2] = getarg;
   vector[3] = yield12;
   vector[4] = yield21;
   vector[5] = uexit;
   char* f1name = "q3prog1";
   if(read_elf(f1name,proc1) != 1)
       printf("Error: Could not read q3prog1 ELF header\n");

   char* f2name = "q3prog2";
   if(read_elf(f2name,proc2) != 1)
       printf("Error: Could not read q3prog2 ELF header\n");

   stack_q3p1 = setup_stack(proc1_stack,proc1);
   stack_q3p2 = setup_stack(proc2_stack,proc2);

   do_switch(&stack_main,stack_q3p1);
}


/***********************************************/
/*********** Your code ends here ***************/
/***********************************************/
