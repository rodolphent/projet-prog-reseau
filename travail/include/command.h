#ifndef COMMAND_H
#define COMMAND_H

/* Découpe une ligne saisie. Retourne le type de message, ou -1 si invalide. */
int get_msg_type_and_infos(char *buffer,
						   char **msg_infos, char **msg_content);

/* Lit stdin et envoie la commande au serveur. Retourne 0 pour arrêter. */
int handle_stdin_input(int sockfd, char *nickname, char *buffer);

#endif
