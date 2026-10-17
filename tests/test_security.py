#!/usr/bin/env python3
"""
test_security.py
Security validation suite testing boundaries, path traversal, and method enforcement.

Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
         from Scratch Using Raw TCP Sockets
"""

import socket
import unittest


class TestServerSecurity(unittest.TestCase):
    HOST = "127.0.0.1"
    PORT = 8080

    def check_server_available(self):
        try:
            s = socket.create_connection((self.HOST, self.PORT), timeout=0.5)
            s.close()
            return True
        except (socket.error, ConnectionRefusedError):
            return False

    def test_path_traversal_blocked(self):
        """Verify directory traversal is strictly blocked (expect 403 or 404)."""
        if not self.check_server_available():
            self.skipTest("HTTP server not actively listening.")

        with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as sock:
            req = b"GET /../../../../etc/passwd HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
            sock.sendall(req)
            resp = sock.recv(1024)
            self.assertTrue(b"403 Forbidden" in resp or b"404 Not Found" in resp)

    def test_url_encoded_traversal_blocked(self):
        """Verify URL-encoded dot-dot traversal (%2e%2e) is strictly blocked."""
        if not self.check_server_available():
            self.skipTest("HTTP server not actively listening.")

        with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as sock:
            req = b"GET /%2e%2e/%2e%2e/etc/passwd HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
            sock.sendall(req)
            resp = sock.recv(1024)
            self.assertIn(b"403 Forbidden", resp)

    def test_method_not_allowed_on_static_asset(self):
        """POST, PUT, or DELETE on static assets must return 405 Method Not Allowed."""
        if not self.check_server_available():
            self.skipTest("HTTP server not actively listening.")

        with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as sock:
            req = b"POST /index.html HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"
            sock.sendall(req)
            resp = sock.recv(1024)
            self.assertIn(b"405 Method Not Allowed", resp)

    def test_unimplemented_http_method(self):
        """Unsupported HTTP method (e.g. TRACE) must return 501 Not Implemented."""
        if not self.check_server_available():
            self.skipTest("HTTP server not actively listening.")

        with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as sock:
            req = b"TRACE / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
            sock.sendall(req)
            resp = sock.recv(1024)
            self.assertIn(b"501 Not Implemented", resp)


if __name__ == "__main__":
    unittest.main()
