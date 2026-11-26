#include <stdio.h>
#include <stdlib.h>
#include "common.h"

void die(int ret_value, const char *msg)
{
	if (ret_value < 0)
	{
		perror(msg);
		exit(EXIT_FAILURE);
	}
}
