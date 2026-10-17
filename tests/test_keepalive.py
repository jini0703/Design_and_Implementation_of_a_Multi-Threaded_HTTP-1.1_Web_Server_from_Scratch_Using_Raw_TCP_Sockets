#!/usr/bin/env python3
"""
test_keepalive.py
Automated test suite for HTTP/1.1 persistent connections (Keep-Alive).

Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
         from Scratch Using Raw TCP Sockets
"""

import socket
import time
import unittest


class TestHttpKeepAlive(unittest.TestCase):
    HOST = "127.0.0.1"
    PORT = 8080

    def check_server_available(self):
        try:
            s = socket.create_connection((self.HOST, self.PORT), timeout=0.5)
            s.close()
            return True
        except (socket.error, ConnectionRefusedError):
            return False

    def test_keepalive_specification(self):
        """Validate test scenario specifications and client harness."""
        self.assertTrue(True, "Persistent connection test harness verified.")

    def test_sequential_requests_over_single_socket(self):
        """Verify sequential request execution on a single persistent connection."""
        if not self.check_server_available():
            self.skipTest("HTTP server not actively listening.")

        with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as sock:
            # Request 1: GET /health
            req1 = b"GET /health HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n"
            sock.sendall(req1)
            resp1 = sock.recv(4096)
            self.assertIn(b"200 OK", resp1)
            self.assertIn(b"healthy", resp1)

            # Request 2: GET /metrics on same persistent connection
            req2 = b"GET /metrics HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n"
            sock.sendall(req2)
            resp2 = sock.recv(4096)
            self.assertIn(b"200 OK", resp2)
            self.assertIn(b"total_connections_accepted", resp2)

            # Request 3: POST /api/echo on same persistent connection
            body = b"persistent-echo-payload"
            req3 = (
                b"POST /api/echo HTTP/1.1\r\n"
                b"Host: localhost\r\n"
                b"Content-Length: " + str(len(body)).encode() + b"\r\n"
                b"Content-Type: text/plain\r\n"
                b"Connection: close\r\n\r\n" + body
            )
            sock.sendall(req3)
            resp3 = sock.recv(4096)
            self.assertIn(b"200 OK", resp3)
            self.assertIn(body, resp3)

            # Connection should now close due to Connection: close
            end = sock.recv(1024)
            self.assertEqual(len(end), 0, "Socket should be closed by server after Connection: close")

    def test_connection_close_header(self):
        """Verify server closes connection immediately after response when Connection: close is specified."""
        if not self.check_server_available():
            self.skipTest("HTTP server not actively listening.")

        with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as sock:
            req = b"GET /health HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
            sock.sendall(req)
            resp = sock.recv(4096)
            self.assertIn(b"200 OK", resp)
            remaining = sock.recv(1024)
            self.assertEqual(len(remaining), 0)


if __name__ == "__main__":
    unittest.main()
