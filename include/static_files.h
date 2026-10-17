#ifndef STATIC_FILES_H
#define STATIC_FILES_H

/**
 * @file static_files.h
 * @brief Secure static file serving, MIME detection, and path traversal defense.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 */

#include "http_response.h"

/**
 * @brief Resolve and prepare a static file response with security sandboxing.
 * @param public_root Base public root directory.
 * @param uri Requested URI path.
 * @param res HTTP response struct to populate.
 * @return 0 on success (or handled error response), negative on fatal error.
 */
int static_files_serve(const char *public_root, const char *uri, HttpResponse *res);

/**
 * @brief Determine standard MIME type from filename extension.
 * @param filepath File path or name.
 * @return MIME type string literal.
 */
const char *static_files_mime_type(const char *filepath);

#endif /* STATIC_FILES_H */
