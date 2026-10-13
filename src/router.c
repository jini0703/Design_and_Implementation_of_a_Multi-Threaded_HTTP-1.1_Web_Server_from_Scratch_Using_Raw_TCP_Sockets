/**
 * @file router.c
 * @brief HTTP request dispatching, URI routing, and endpoint handler mapping.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 *
 * Intended Responsibilities:
 *  1. Request URI Dispatching:
 *     - Parse requested path and match against configured route tables.
 *  2. Built-in Endpoints:
 *     - '/metrics': Generates real-time JSON metrics snapshot for monitoring dashboard.
 *     - '/health' or '/status': Lightweight liveness and readiness probe for load balancers.
 *  3. Static Asset Delegation:
 *     - Delegate standard resource paths to the static file handler (static_files.c).
 *  4. Fallback Error Handlers:
 *     - Dispatch HTTP 404 (Not Found) for unmapped routes.
 *     - Dispatch HTTP 405 (Method Not Allowed) for unsupported verbs on defined paths.
 *
 * Implementation Status:
 *  - Route dispatch architecture designed; handlers scheduled for development in Phase 3.
 */

#include "http_parser.h"
#include "http_response.h"

/* Router interface declarations for integration */
int router_dispatch(const HttpRequest *req, HttpResponse *res);

int router_dispatch(const HttpRequest *req, HttpResponse *res) {
    if (!req || !res) return -1;

    /* Handler dispatching logic will be implemented in Phase 3 */
    http_response_build_error(res, HTTP_STATUS_NOT_FOUND, "Resource not found on this server.");
    return 0;
}
