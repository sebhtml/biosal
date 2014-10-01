
#include "test.h"

#include <core/file_storage/input/buffered_reader.h>

#define BUFFER_SIZE 128000

int main(int argc, char **argv)
{
    BEGIN_TESTS();

    struct core_buffered_reader reader;
    char file[] = "/home/boisvert/dropbox/SRS213780-Ecoli/SRR306102_1.fastq.gz";
    char buffer[BUFFER_SIZE];
    int expected;
    int actual;

    expected = 17222992;

    core_buffered_reader_init(&reader, file, 0);

    actual = 0;

    while (core_buffered_reader_read_line(&reader, buffer, BUFFER_SIZE) > 0) {

        ++actual;

            /*
        printf("Line= %s\n", buffer);
        */
    }

    TEST_INT_EQUALS(actual, expected);

    core_buffered_reader_destroy(&reader);

    END_TESTS();

    return 0;
}


