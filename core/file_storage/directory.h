
#ifndef CORE_DIRECTORY_H
#define CORE_DIRECTORY_H

/*
 * Returns 1 if it exists, 0 otherwise
 */
int core_directory_verify_existence(const char *directory);

/*
 * Returns 1 if created, 0 otherwise
 */
int core_directory_create(const char *directory);

#endif
