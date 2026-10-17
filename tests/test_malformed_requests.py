#!/usr/bin/env python3
"""
test_malformed_requests.py
Defensive parsing and malformed HTTP/1.1 request fuzzing suite.

Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
         from Scratch Using Raw TCP Sockets
"""

import socket
import unittest


class TestMalformedRequests(unittest.TestCase):
    HOST = "127.0.0.1"
    PORT = 8080

    def check_server_available(self):
        try:
            s = socket.create_connection((self.HOST, self.PORT), timeout=0.5)
            s.close()
            return True
        except (socket.error, ConnectionRefusedError):
            return False

    def test_random_bytes_rejection(self):
        """Verify the server responds with 400 Bad Request or drops connection cleanly."""
        if not self.check_server_available():
            self.skipTest("HTTP server not actively listening.")

        with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as sock:
            sock.sendall(b"\xde\xad\xbe\xef\x00\xff\xfe\xfd\r\n\r\n")
            response = sock.recv(1024)
            self.assertTrue(b"400" in response or len(response) == 0)

    def test_missing_host_header_400(self):
        """HTTP/1.1 requires Host header; absence must yield 400 Bad Request."""
        if not self.check_server_available():
            self.skipTest("HTTP server not actively listening.")

        with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as sock:
            req = b"GET /health HTTP/1.1\r\nUser-Agent: fuzz\r\nConnection: close\r\n\r\n"
            sock.sendall(req)
            resp = sock.recv(4096)
            self.assertIn(b"400 Bad Request", resp)

    def test_conflicting_content_length_transfer_encoding(self):
        """Conflicting framing (Content-Length + Transfer-Encoding) must yield 400 Bad Request."""
        if not self.check_server_available():
            self.skipTest("HTTP server not actively listening.")

        with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as sock:
            req = (
                b"POST /api/echo HTTP/1.1\r\n"
                b"Host: localhost\r\n"
                b"Content-Length: 5\r\n"
                b"Transfer-Encoding: chunked\r\n"
                b"Connection: close\r\n\r\n"
                b"12345"
            )
            sock.sendall(req)
            resp = sock.recv(4096)
            self.assertIn(b"400 Bad Request", resp)

    def test_chunked_transfer_encoding_501(self):
        """Transfer-Encoding: chunked is deliberately not implemented; must yield 501 Not Implemented."""
        if not self.check_server_available():
            self.skipTest("HTTP server not actively listening.")

        with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as sock:
            req = (
                b"POST /api/echo HTTP/1.1\r\n"
                b"Host: localhost\r\n"
                b"Transfer-Encoding: chunked\r\n"
                b"Connection: close\r\n\r\n"
                b"0\r\n\r\n"
            )
            sock.sendall(req)
            resp = sock.recv(4096)
            self.assertIn(b"501 Not Implemented", resp)

    def test_expect_continue_417(self):
        """Expect: 100-continue must yield 417 Expectation Failed."""
        if not self.check_server_available():
            self.skipTest("HTTP server not actively listening.")

        with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as sock:
            req = (
                b"POST /api/echo HTTP/1.1\r\n"
                b"Host: localhost\r\n"
                b"Content-Length: 5\r\n"
                b"Expect: 100-continue\r\n"
                b"Connection: close\r\n\r\n"
            )
            sock.sendall(req)
            resp = sock.recv(4096)
            self.assertIn(b"417 Expectation Failed", resp)


if __name__ == "__main__":
    unittest.main()
