#ifndef ROUTER_H
#define ROUTER_H

/**
 * @file router.h
 * @brief HTTP request dispatching, URI routing, and endpoint handler mapping.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 */

#include "http_parser.h"
#include "http_response.h"

/* Forward declaration */
typedef struct ServerContext ServerContext;

/**
 * @brief Dispatch an incoming HTTP request to the appropriate route handler.
 * @param ctx Server context containing config, metrics, and controller.
 * @param req Parsed incoming HTTP request.
 * @param res Response structure to populate.
 * @return 0 on success, negative on error.
 */
int router_dispatch(ServerContext *ctx, const HttpRequest *req, HttpResponse *res);

#endif /* ROUTER_H */
