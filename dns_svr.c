#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {

    unsigned char size_header[2];
    if (read(STDIN_FILENO, size_header, 2) == -1) {
        return -1;
    }
    size_t size = (size_header[0] << 8) + (size_header[1]);
    printf("%lu\n", size);

    unsigned char *buf = malloc(size * sizeof(*buf));
    while(read(STDIN_FILENO, buf, 1) > 0) {
        printf("%x ", *buf);
    }
    printf("\n");
    free(buf);

    return 0;
}
