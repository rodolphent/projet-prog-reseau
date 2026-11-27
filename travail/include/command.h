#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>
#include "common.h"
#include "msg_struct.h"

struct file_transfer {
    int pending;
    int listen_fd; // -1 si pas en attente 
    char sender[NICK_LEN];  /* qui veut envoyer le fichier */
    char receiver[NICK_LEN];  /* à qui on veut envoyer un fichier */
    char filepath[MSG_LEN];   /* fichier à envoyer, "" si aucun */
};

void send_message(int sockfd, const char *nickname, enum msg_type type,
                  const char *infos, const char *content);
				  
/* Découpe une ligne saisie. Retourne le type de message, ou -1 si invalide. */
int get_msg_type_and_infos(char *buffer,
						   char **msg_infos, char **msg_content);

/* Lit stdin et envoie la commande au serveur. Retourne 0 pour arrêter. */
int handle_stdin_input(int sockfd, char *nickname, char *buffer, struct file_transfer *ft);

/* Reçoit exactement length octets. Retourne 0 en cas d'échec. */
int receive_all(int sockfd, void *buffer, size_t length);

#endif
