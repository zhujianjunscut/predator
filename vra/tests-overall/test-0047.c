/**
* @file test-0047.c
*
* @brief Simple loop.
*/

int main(int argc, const char *argv[]) {
	int buffer[10];
	int i = 0;

	while (i < 10) {
		buffer[i] = i;
	}

	return 0;
}

