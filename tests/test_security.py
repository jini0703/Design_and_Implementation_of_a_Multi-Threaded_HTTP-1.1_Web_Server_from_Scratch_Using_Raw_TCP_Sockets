#!/usr/bin/env python3
"""
test_security.py
Security validation suite testing boundaries, path traversal, and method enforcement.

Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
         from Scratch Using Raw TCP Sockets
"""

import os
import socket
import unittest

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))


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

    def test_traversal_with_query_string_blocked(self):
        """Verify directory traversal is strictly blocked even when query string is attached."""
        if not self.check_server_available():
            self.skipTest("HTTP server not actively listening.")

        # Traversal in path with query string
        with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as sock:
            req = b"GET /../../../../etc/passwd?test=1 HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
            sock.sendall(req)
            resp = sock.recv(1024)
            self.assertTrue(b"403 Forbidden" in resp or b"404 Not Found" in resp)

        # URL-encoded traversal with query string
        with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as sock:
            req = b"GET /%2e%2e/%2e%2e/etc/passwd?test=1 HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
            sock.sendall(req)
            resp = sock.recv(1024)
            self.assertIn(b"403 Forbidden", resp)

    def test_nul_byte_injection_blocked(self):
        """Verify NUL byte injection (%00) is strictly rejected with 400 Bad Request."""
        if not self.check_server_available():
            self.skipTest("HTTP server not actively listening.")

        with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as sock:
            req = b"GET /index.html%00.txt HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
            sock.sendall(req)
            resp = sock.recv(1024)
            self.assertIn(b"400 Bad Request", resp)

    def test_backslash_traversal_blocked(self):
        """Verify backslash traversal attempts are strictly blocked with 400 or 403."""
        if not self.check_server_available():
            self.skipTest("HTTP server not actively listening.")

        with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as sock:
            req = b"GET /..\\..\\etc\\passwd HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
            sock.sendall(req)
            resp = sock.recv(1024)
            self.assertTrue(b"400 Bad Request" in resp or b"403 Forbidden" in resp)

    def test_malformed_url_encoding_rejected(self):
        """Verify incomplete or invalid percent-encodings return 400 Bad Request."""
        if not self.check_server_available():
            self.skipTest("HTTP server not actively listening.")

        with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as sock:
            req = b"GET /index%2g.html HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
            sock.sendall(req)
            resp = sock.recv(1024)
            self.assertIn(b"400 Bad Request", resp)

    def test_symlink_directory_index_traversal_blocked(self):
        """Verify symlinked index.html pointing outside document root is strictly blocked with 403."""
        if not self.check_server_available():
            self.skipTest("HTTP server not actively listening.")

        test_dir = os.path.join(PROJECT_ROOT, "public", "test_symlink_dir")
        symlink_path = os.path.join(test_dir, "index.html")
        try:
            os.makedirs(test_dir, exist_ok=True)
            if os.path.islink(symlink_path) or os.path.exists(symlink_path):
                os.remove(symlink_path)
            os.symlink("/etc/passwd", symlink_path)

            with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as sock:
                req = b"GET /test_symlink_dir/ HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
                sock.sendall(req)
                resp = sock.recv(1024)
                self.assertIn(b"403 Forbidden", resp)
                self.assertNotIn(b"root:", resp)
        finally:
            if os.path.islink(symlink_path) or os.path.exists(symlink_path):
                try:
                    os.remove(symlink_path)
                except Exception:
                    pass
            if os.path.exists(test_dir):
                try:
                    os.rmdir(test_dir)
                except Exception:
                    pass

    def test_url_encoded_filename_serving(self):
        """Verify that files with encoded spaces (%20) in their filename are correctly served."""
        if not self.check_server_available():
            self.skipTest("HTTP server not actively listening.")

        test_file = os.path.join(PROJECT_ROOT, "public", "test spaced file.txt")
        try:
            with open(test_file, "w", encoding="utf-8") as f:
                f.write("hello-encoded-space-content")

            with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as sock:
                req = b"GET /test%20spaced%20file.txt HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
                sock.sendall(req)
                resp = b""
                while True:
                    chunk = sock.recv(1024)
                    if not chunk:
                        break
                    resp += chunk
                self.assertIn(b"200 OK", resp)
                self.assertIn(b"hello-encoded-space-content", resp)
        finally:
            if os.path.exists(test_file):
                try:
                    os.remove(test_file)
                except Exception:
                    pass

    def test_benign_dot_dot_in_filename(self):
        """Verify filenames containing '..' not as directory segments (e.g. release..txt, foo..bar) are served."""
        if not self.check_server_available():
            self.skipTest("HTTP server not actively listening.")

        f1 = os.path.join(PROJECT_ROOT, "public", "release..txt")
        f2 = os.path.join(PROJECT_ROOT, "public", "foo..bar.txt")
        try:
            with open(f1, "w", encoding="utf-8") as f:
                f.write("release-content")
            with open(f2, "w", encoding="utf-8") as f:
                f.write("foobar-content")

            with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as sock:
                req = b"GET /release..txt HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
                sock.sendall(req)
                resp = b""
                while True:
                    chunk = sock.recv(1024)
                    if not chunk:
                        break
                    resp += chunk
                self.assertIn(b"200 OK", resp)
                self.assertIn(b"release-content", resp)

            with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as sock:
                req = b"GET /foo..bar.txt HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
                sock.sendall(req)
                resp = b""
                while True:
                    chunk = sock.recv(1024)
                    if not chunk:
                        break
                    resp += chunk
                self.assertIn(b"200 OK", resp)
                self.assertIn(b"foobar-content", resp)
        finally:
            for p in (f1, f2):
                if os.path.exists(p):
                    try:
                        os.remove(p)
                    except Exception:
                        pass

    def test_case_insensitive_encoded_traversal(self):
        """Verify uppercase and mixed-case percent-encoded traversal segments are blocked."""
        if not self.check_server_available():
            self.skipTest("HTTP server not actively listening.")

        for path in (b"/%2E%2E/etc/passwd", b"/.%2e/etc/passwd", b"/%2e./etc/passwd"):
            with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as sock:
                req = b"GET " + path + b" HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
                sock.sendall(req)
                resp = sock.recv(1024)
                self.assertTrue(b"403 Forbidden" in resp or b"404 Not Found" in resp)
                self.assertNotIn(b"root:", resp)


if __name__ == "__main__":
    unittest.main()
