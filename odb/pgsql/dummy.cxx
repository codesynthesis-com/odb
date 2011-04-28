#include <libpq-fe.h>

PGconn* dummy ()
{
  return PQconnectdb ("connection info");
}
