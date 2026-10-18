/**
 * @file static_files.c
 * @brief Secure static file serving, MIME type detection, and path traversal prevention.
 *
 * Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
 *          from Scratch Using Raw TCP Sockets
 */

#include "static_files.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

const char *static_files_mime_type(const char *filepath) {
    if (!filepath) return "application/octet-stream";

    const char *ext = strrchr(filepath, '.');
    if (!ext) return "application/octet-stream";

    if (strcasecmp(ext, ".html") == 0 || strcasecmp(ext, ".htm") == 0) {
        return "text/html; charset=utf-8";
    } else if (strcasecmp(ext, ".css") == 0) {
        return "text/css; charset=utf-8";
    } else if (strcasecmp(ext, ".js") == 0) {
        return "application/javascript; charset=utf-8";
    } else if (strcasecmp(ext, ".json") == 0) {
        return "application/json; charset=utf-8";
    } else if (strcasecmp(ext, ".png") == 0) {
        return "image/png";
    } else if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0) {
        return "image/jpeg";
    } else if (strcasecmp(ext, ".gif") == 0) {
        return "image/gif";
    } else if (strcasecmp(ext, ".svg") == 0) {
        return "image/svg+xml";
    } else if (strcasecmp(ext, ".ico") == 0) {
        return "image/x-icon";
    } else if (strcasecmp(ext, ".txt") == 0) {
        return "text/plain; charset=utf-8";
    } else if (strcasecmp(ext, ".pdf") == 0) {
        return "application/pdf";
    } else if (strcasecmp(ext, ".woff") == 0) {
        return "font/woff";
    } else if (strcasecmp(ext, ".woff2") == 0) {
        return "font/woff2";
    }

    return "application/octet-stream";
}

static int hex_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int url_decode(const char *in, char *out, size_t out_cap) {
    if (!in || !out || out_cap == 0) return -1;
    size_t o = 0;
    for (size_t i = 0; in[i] != '\0'; i++) {
        char c = in[i];
        if (c == '%') {
            if (in[i+1] == '\0' || in[i+2] == '\0') {
                return -1; /* Incomplete hex sequence */
            }
            int h1 = hex_val(in[i+1]);
            int h2 = hex_val(in[i+2]);
            if (h1 < 0 || h2 < 0) {
                return -1; /* Invalid hex digits */
            }
            char decoded = (char)((h1 << 4) | h2);
            if (decoded == '\0') {
                return -1; /* NUL byte injection attempt */
            }
            c = decoded;
            i += 2;
        }
        if (c == '\\') {
            return -1; /* Prohibit backslashes in request path */
        }
        if (o + 1 >= out_cap) {
            return -1; /* Destination buffer overflow */
        }
        out[o++] = c;
    }
    out[o] = '\0';
    return 0;
}

static bool has_traversal_segment(const char *path) {
    if (!path) return false;
    const char *p = path;
    while (*p != '\0') {
        while (*p == '/' || *p == '\\') {
            p++;
        }
        if (*p == '\0') break;
        const char *start = p;
        while (*p != '\0' && *p != '/' && *p != '\\') {
            p++;
        }
        size_t seg_len = (size_t)(p - start);
        if (seg_len == 2 && start[0] == '.' && start[1] == '.') {
            return true;
        }
        if (seg_len == 6 && strncasecmp(start, "%2e%2e", 6) == 0) {
            return true;
        }
        if (seg_len == 4 && (strncasecmp(start, "%2e.", 4) == 0 || strncasecmp(start, ".%2e", 4) == 0)) {
            return true;
        }
    }
    return false;
}

int static_files_serve(const char *public_root, const char *uri, HttpResponse *res) {
    if (!public_root || !uri || !res) return -1;

    /* Detect raw directory traversal markers or encoded dot-dot segments before resolution */
    if (has_traversal_segment(uri)) {
        http_response_build_error(res, HTTP_STATUS_FORBIDDEN,
                                  "Path traversal sequence detected.", res->is_keep_alive);
        return 0;
    }

    /* Strip query string and fragment */
    char clean_uri[512];
    size_t ulen = 0;
    while (uri[ulen] && uri[ulen] != '?' && uri[ulen] != '#' && ulen < sizeof(clean_uri) - 1) {
        clean_uri[ulen] = uri[ulen];
        ulen++;
    }
    clean_uri[ulen] = '\0';

    /* Decode percent-encoding and reject NUL bytes or illegal characters */
    char decoded_uri[512];
    if (url_decode(clean_uri, decoded_uri, sizeof(decoded_uri)) != 0) {
        http_response_build_error(res, HTTP_STATUS_BAD_REQUEST,
                                  "Malformed URI encoding or illegal character detected.", res->is_keep_alive);
        return 0;
    }

    /* Verify no directory traversal segments exist after URL decoding */
    if (has_traversal_segment(decoded_uri)) {
        http_response_build_error(res, HTTP_STATUS_FORBIDDEN,
                                  "Path traversal sequence detected.", res->is_keep_alive);
        return 0;
    }

    /* Skip leading slashes */
    const char *rel_path = decoded_uri;
    while (*rel_path == '/') {
        rel_path++;
    }
    if (*rel_path == '\0') {
        rel_path = "index.html";
    }

    /* Resolve canonical root */
    char canonical_root[PATH_MAX];
    if (realpath(public_root, canonical_root) == NULL) {
        http_response_build_error(res, HTTP_STATUS_INTERNAL_ERROR,
                                  "Public document root inaccessible.", res->is_keep_alive);
        return 0;
    }

    char candidate[PATH_MAX];
    int written = snprintf(candidate, sizeof(candidate), "%s/%s", canonical_root, rel_path);
    if (written < 0 || (size_t)written >= sizeof(candidate)) {
        http_response_build_error(res, HTTP_STATUS_URI_TOO_LONG,
                                  "Constructed file path exceeds system limits.", res->is_keep_alive);
        return 0;
    }

    char canonical_target[PATH_MAX];
    if (realpath(candidate, canonical_target) == NULL) {
        if (errno == ENOENT) {
            http_response_build_error(res, HTTP_STATUS_NOT_FOUND,
                                      "Requested resource not found.", res->is_keep_alive);
        } else if (errno == EACCES) {
            http_response_build_error(res, HTTP_STATUS_FORBIDDEN,
                                      "Permission denied accessing resource.", res->is_keep_alive);
        } else {
            http_response_build_error(res, HTTP_STATUS_NOT_FOUND,
                                      "Resource resolution failed.", res->is_keep_alive);
        }
        return 0;
    }

    /* Ensure canonical target is strictly within canonical root sandbox */
    size_t root_len = strlen(canonical_root);
    if (strncmp(canonical_target, canonical_root, root_len) != 0 ||
        (canonical_target[root_len] != '/' && canonical_target[root_len] != '\0')) {
        http_response_build_error(res, HTTP_STATUS_FORBIDDEN,
                                  "Access forbidden outside document root.", res->is_keep_alive);
        return 0;
    }

    struct stat st;
    if (stat(canonical_target, &st) != 0) {
        http_response_build_error(res, HTTP_STATUS_NOT_FOUND,
                                  "File stat failed.", res->is_keep_alive);
        return 0;
    }

    char final_file[PATH_MAX];
    if (S_ISDIR(st.st_mode)) {
        /* Directory requested: check for index.html inside */
        int idx_w = snprintf(final_file, sizeof(final_file), "%s/index.html", canonical_target);
        if (idx_w < 0 || (size_t)idx_w >= sizeof(final_file)) {
            http_response_build_error(res, HTTP_STATUS_URI_TOO_LONG,
                                      "Path exceeds limits.", res->is_keep_alive);
            return 0;
        }

        /* Resolve realpath of index.html to verify symlinks cannot escape sandbox */
        char canonical_index[PATH_MAX];
        if (realpath(final_file, canonical_index) == NULL) {
            http_response_build_error(res, HTTP_STATUS_FORBIDDEN,
                                      "Directory listing forbidden.", res->is_keep_alive);
            return 0;
        }

        if (strncmp(canonical_index, canonical_root, root_len) != 0 ||
            (canonical_index[root_len] != '/' && canonical_index[root_len] != '\0')) {
            http_response_build_error(res, HTTP_STATUS_FORBIDDEN,
                                      "Access forbidden outside document root.", res->is_keep_alive);
            return 0;
        }

        struct stat idx_st;
        if (stat(canonical_index, &idx_st) == 0 && S_ISREG(idx_st.st_mode)) {
            st = idx_st;
            strncpy(final_file, canonical_index, sizeof(final_file) - 1);
            final_file[sizeof(final_file) - 1] = '\0';
        } else {
            /* Directory listing is strictly forbidden */
            http_response_build_error(res, HTTP_STATUS_FORBIDDEN,
                                      "Directory listing forbidden.", res->is_keep_alive);
            return 0;
        }
    } else if (S_ISREG(st.st_mode)) {
        strncpy(final_file, canonical_target, sizeof(final_file) - 1);
        final_file[sizeof(final_file) - 1] = '\0';
    } else {
        http_response_build_error(res, HTTP_STATUS_FORBIDDEN,
                                  "Special file type access forbidden.", res->is_keep_alive);
        return 0;
    }

    int fd = open(final_file, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) {
        if (errno == ELOOP || errno == EMLINK) {
            http_response_build_error(res, HTTP_STATUS_FORBIDDEN,
                                      "Symlink traversal forbidden.", res->is_keep_alive);
        } else if (errno == EACCES) {
            http_response_build_error(res, HTTP_STATUS_FORBIDDEN,
                                      "File read permission denied.", res->is_keep_alive);
        } else {
            http_response_build_error(res, HTTP_STATUS_INTERNAL_ERROR,
                                      "Unable to open file.", res->is_keep_alive);
        }
        return 0;
    }

    struct stat opened_st;
    if (fstat(fd, &opened_st) != 0 || !S_ISREG(opened_st.st_mode) ||
        opened_st.st_dev != st.st_dev || opened_st.st_ino != st.st_ino) {
        close(fd);
        http_response_build_error(res, HTTP_STATUS_FORBIDDEN,
                                  "Access forbidden outside document root.", res->is_keep_alive);
        return 0;
    }

    const char *mime = static_files_mime_type(final_file);
    http_response_set_status(res, HTTP_STATUS_OK);
    http_response_set_file(res, mime, fd, (size_t)opened_st.st_size);

    return 0;
}
