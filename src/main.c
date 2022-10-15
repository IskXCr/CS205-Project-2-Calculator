/*  This file is the main program of the Simple Arithmetic Program, acting
    as a bridge to connect the user to the interface of the SAP virtual machine.
 *******************************************************************/

#include "sapdefs.h"
#include "sap.h"
#include "global.h"
#include "utils.h"
#include "opt.h"

#define DEBUG

#ifdef DEBUG
#include "test.h"
#endif

/* Definition of constants */
int quiet = 0;

#define OPT_CNT 3
static option options[OPT_CNT] = {
    {'h', "help"},
    {'q', "quiet"},
    {'v', "version"}};

static void
usage(const char *progname)
{
    printf("usage: %s [options] [file ...]\n%s%s%s", progname,
           "  -h  --help     print this usage and exit\n",
           "  -q  --quiet    don't print initial banner\n",
           "  -v  --version  print version information and exit\n");
}

static void
version()
{
    printf("Simple Arithmetic Program, aka SAP. VERSION: " VERSION "\nEngineering sample. Interactive mode only.\n");
}

/* Process argument to apply the settings. */
static void
_process_arg_abbr(char arg, char *progname)
{
    switch (arg)
    {
    case 'h':
        usage(progname);
        exit(0);
    case 'q':
        quiet = TRUE;
        break;
    case 'v':
        version();
        exit(0);
    default:
        usage(progname);
        exit(1);
    }
}

static void
_parse_args_full(char *arg, char *progname)
{
    if (arg == NULL)
    {
        usage(progname);
        exit(1);
    }
    int index;
    for (index = 0; index < OPT_CNT; ++index)
        if (strcmp(options[index].full, arg) == 0)
            break;
    if (index == OPT_CNT)
    {
        usage(progname);
        exit(1);
    }
    _process_arg_abbr(options[index].abbr, progname);
}

/* Parse arguments from the command line. */
static void
parse_args(int argc, char **argv)
{
    int opt;
    char **p = argv;
    while (--argc > 0)
    {
        ++p;
        switch (opt = **p)
        {
        case '-':
            int c = *(++*p);
            if (c == '-')
            {
                char *arg = fetch_token(++*p);
                // parse arg
                _parse_args_full(arg, argv[0]);
            }
            else
                while (c != '\0')
                {
                    _process_arg_abbr(c, argv[0]);
                    c = *(++*p);
                }
            break;
        default:
            usage(argv[0]);
            exit(1);
        }
    }
}

int main(int argc, char **argv)
{
    parse_args(argc, argv);
    if (quiet != TRUE)
        version();
    void handle(void);
    init_utils_lib(&handle);
    test();
}

void handle(void)
{
    exit(1);
}