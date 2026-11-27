#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include "command.h"
#include "common.h"
#include "file_transfer.h"
#include "msg_struct.h"

int send_all(int sockfd, const void *buffer, size_t length)
{
	size_t sent = 0;
	while (sent < length)
	{
		ssize_t n = send(sockfd, (const char *)buffer + sent, length - sent, MSG_NOSIGNAL);
		if (n < 0)
		{
			perror("send to peer");
			return 0;
		}
		sent += (size_t)n;
	}
	return 1;
}

int send_frame(int sockfd, const char *nickname, enum msg_type type, const char *infos, const char *content, int length)
{
	struct message msg;
	memset(&msg, 0, sizeof(msg));
	msg.pld_len = length;
	msg.type = type;
	snprintf(msg.nick_sender, sizeof(msg.nick_sender), "%s", nickname);
	snprintf(msg.infos, sizeof(msg.infos), "%s", infos);
	return send_all(sockfd, &msg, sizeof(msg)) && send_all(sockfd, content, length);
}

char *get_filename(char *path)
{
	char *slash = strrchr(path, '/');
	if (slash == NULL)
	{
		return path;
	}
	return slash + 1;
}

int prepare_file_request(const char *nickname, const char *receiver,
						 char **msg_content, struct file_transfer *ft)
{
	char *path = *msg_content;

	// Enlever les guillemets autour du chemin
	if (path[0] == '"')
	{
		path++;
		path[strcspn(path, "\"")] = '\0';
	}

	if (nickname[0] == '\0')
	{
		printf("Please choose a nickname first with /nick\n");
		return 0;
	}

	if (strcmp(receiver, nickname) == 0)
	{
		printf("You cannot send a file to yourself\n");
		return 0;
	}

	FILE *file = fopen(path, "rb");
	if (file == NULL)
	{
		perror("open file");
		return 0;
	}
	fclose(file);

	snprintf(ft->receiver, sizeof(ft->receiver), "%s", receiver);
	snprintf(ft->filepath, sizeof(ft->filepath), "%s", path);
	*msg_content = get_filename(ft->filepath);
	return 1;
}

int connect_to_peer(char *endpoint)
{
	char *port = strchr(endpoint, ':');
	if (port == NULL)
	{
		return -1;
	}
	*port++ = '\0';

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(port));
	if (inet_pton(AF_INET, endpoint, &addr.sin_addr) != 1)
	{
		return -1;
	}

	int sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0)
	{
		perror("socket");
		return -1;
	}
	if (connect(sfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		perror("connect to peer");
		close(sfd);
		return -1;
	}
	return sfd;
}

int send_file(int sockfd, const char *nickname, char *path)
{
	FILE *file = fopen(path, "rb");
	if (file == NULL)
	{
		perror("open file");
		return 0;
	}

	char *filename = get_filename(path);
	char chunk[MSG_LEN];
	int n;

	while ((n = fread(chunk, 1, MSG_LEN, file)) > 0)
	{
		if (!send_frame(sockfd, nickname, FILE_SEND, filename, chunk, n))
		{
			fclose(file);
			return 0;
		}
	}
	fclose(file);

	return send_frame(sockfd, nickname, FILE_SEND, filename, "", 0);
}

void handle_file_accept(const char *nickname, const char *receiver, char *endpoint, struct file_transfer *ft)
{
	int sfd = connect_to_peer(endpoint);

	if (ft->filepath[0] == '\0' || strcmp(receiver, ft->receiver) != 0)
	{
		// Ancienne demande : on ferme la connexion pour libérer le récepteur
		if (sfd >= 0)
		{
			close(sfd);
		}
		return;
	}

	printf("%s accepted file transfer.\n", receiver);
	if (sfd >= 0)
	{
		printf("Connecting to %s and sending the file...\n", receiver);

		// Attendre la confirmation du récepteur
		struct message ack;
		if (send_file(sfd, nickname, ft->filepath) &&
			receive_all(sfd, &ack, sizeof(ack)) && ack.type == FILE_ACK)
		{
			printf("%s has received the file.\n", receiver);
		}
		else
		{
			printf("File transfer failed\n");
		}
		close(sfd);
	}
	ft->filepath[0] = '\0';
}

void handle_file_reject(const char *receiver, struct file_transfer *ft)
{
	printf("%s refused file transfer.\n", receiver);
	if (strcmp(receiver, ft->receiver) == 0)
	{
		ft->filepath[0] = '\0';
	}
}

int save_file(int sockfd, char *filename)
{
	struct message msg;
	char chunk[MSG_LEN];
	char path[MSG_LEN];
	FILE *file = NULL;

	while (receive_all(sockfd, &msg, sizeof(msg)))
	{
		// Vérifier la longueur annoncée avant de lire
		if (msg.type != FILE_SEND || msg.pld_len < 0 || msg.pld_len > MSG_LEN)
		{
			fprintf(stderr, "Invalid file message received\n");
			break;
		}

		// Ouvrir le fichier au premier message
		if (file == NULL)
		{
			msg.infos[INFOS_LEN - 1] = '\0';
			if (strchr(msg.infos, '/') != NULL)
			{
				fprintf(stderr, "Invalid file name\n");
				break;
			}
			snprintf(filename, INFOS_LEN, "%s", msg.infos);
			mkdir(".resa1", 0755); // échoue sans conséquence si le dossier existe
			mkdir(".resa1/inbox", 0755);
			snprintf(path, sizeof(path), ".resa1/inbox/%s", filename);
			file = fopen(path, "wb");
			if (file == NULL)
			{
				perror("open file");
				break;
			}
		}

		// Un message vide indique la fin du fichier
		if (msg.pld_len == 0)
		{
			fclose(file);
			printf("%s saved in %s\n", filename, path);
			return 1;
		}

		if (!receive_all(sockfd, chunk, msg.pld_len))
		{
			break;
		}
		fwrite(chunk, 1, msg.pld_len, file);
	}

	if (file != NULL)
	{
		fclose(file);
	}
	return 0;
}

void receive_file(const char *nickname, struct file_transfer *ft)
{
	int fd = accept(ft->listen_fd, NULL, NULL);
	close(ft->listen_fd);
	ft->listen_fd = -1;
	if (fd < 0)
	{
		perror("accept");
		return;
	}

	printf("Receiving the file from %s...\n", ft->sender);

	char filename[INFOS_LEN];
	if (save_file(fd, filename))
	{
		send_frame(fd, nickname, FILE_ACK, filename, "", 0);
	}
	else
	{
		printf("File transfer failed\n");
	}
	close(fd);
}
