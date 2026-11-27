#ifndef COMMON_H
#define COMMON_H

#define MSG_LEN 1024
#define SERV_PORT "8080"
#define SERV_ADDR "127.0.0.1"

/* Affiche msg avec perror() et quitte le programme si ret_value < 0. */
void die(int ret_value, const char *msg);

#endif
