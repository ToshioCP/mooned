#include "moonedapp.h"

int
main (int argc, char *argv[])
{
  return g_application_run(G_APPLICATION (mooned_application_new ()), argc, argv);
}
