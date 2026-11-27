#ifndef FILE_TRANSFER_H
#define FILE_TRANSFER_H

#include "command.h"

/* Vérifie le fichier d'un /send et mémorise son chemin. msg_content pointe
 * ensuite sur le nom du fichier. Retourne 0 si la demande est invalide. */
int prepare_file_request(const char *nickname, const char *receiver,
						 char **msg_content, struct file_transfer *ft);

/* Côté émetteur : se connecte à endpoint ("ip:port") et envoie le fichier. */
void handle_file_accept(const char *nickname, const char *receiver,
						char *endpoint, struct file_transfer *ft);

/* Côté émetteur : le récepteur a refusé. */
void handle_file_reject(const char *receiver, struct file_transfer *ft);

/* Côté récepteur : accepte la connexion et enregistre le fichier. */
void receive_file(const char *nickname, struct file_transfer *ft);

#endif
