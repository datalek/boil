/*
 * logger.h - Logger port
 *
 * Simple logging interface.
 */

#ifndef BOIL_DOMAIN_LOGGER_H
#define BOIL_DOMAIN_LOGGER_H

typedef struct {
    void (*info)(const char *msg);
    void (*error)(const char *msg);
} Logger;

#endif /* BOIL_DOMAIN_LOGGER_H */
