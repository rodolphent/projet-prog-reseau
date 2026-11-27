#ifndef SERVER_COMMAND_H
#define SERVER_COMMAND_H

#include "utils.h"

/* Traite le message reçu de `client`. `clients` sert au /nick.
 * Retourne 0 pour /quit, 1 sinon. */
int handle_command(struct Client **clients,struct Client *client);

#endif
