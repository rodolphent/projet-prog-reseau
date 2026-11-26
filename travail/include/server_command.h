#ifndef SERVER_COMMAND_H
#define SERVER_COMMAND_H

#include "client_list.h"

/* Traite le message reçu d'un client. Retourne 0 pour /quit, 1 sinon. */
int handle_command(struct Client *client);

#endif
