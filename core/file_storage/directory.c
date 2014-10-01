
#include "directory.h"

#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

int biosal_directory_verify_existence(const char *directory)
{
    int mode;
    int result;

    /*
     * \see http://pubs.opengroup.org/onlinepubs/009695399/functions/access.html
     * \see http://stackoverflow.com/questions/15470639/how-to-check-if-a-file-exists-in-c
     */

    mode = F_OK;

    result = access(directory, mode);

    if (result != -1) {
        return 1;
    }

    return 0;
}

int biosal_directory_create(const char *directory)
{
    int status;

    if (biosal_directory_verify_existence(directory)) {
        return 0;
    }

    /*
     * \see http://pubs.opengroup.org/onlinepubs/009695399/functions/mkdir.html
     *
     * The following creates a directory with read/write/search permissions for owner
     * and group, and with read/search permissions for others.
     */
    status = mkdir(directory, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    if (status == 0) {
        return 1;
    }

    return 0;
}
