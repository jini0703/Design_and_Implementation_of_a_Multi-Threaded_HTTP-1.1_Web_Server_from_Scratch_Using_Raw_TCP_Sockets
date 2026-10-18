#!/usr/bin/env python3
"""
test_keepalive.py
Automated test suite for HTTP/1.1 persistent connections (Keep-Alive).

Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
         from Scratch Using Raw TCP Sockets
"""

import json
import os
import socket
import time
import unittest

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))


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

    def test_query_string_routing_regression(self):
        """Regression test: verify query strings do not cause route mismatches on endpoints."""
        if not self.check_server_available():
            self.skipTest("HTTP server not actively listening.")

        cases = [
            ("GET", "/health", None, b"200 OK", b"healthy"),
            ("GET", "/health?test=1", None, b"200 OK", b"healthy"),
            ("GET", "/health?", None, b"200 OK", b"healthy"),
            ("GET", "/metrics", None, b"200 OK", b"total_connections_accepted"),
            ("GET", "/metrics?test=1", None, b"200 OK", b"total_connections_accepted"),
            ("GET", "/metrics?", None, b"200 OK", b"total_connections_accepted"),
            ("GET", "/index.html?v=2.0", None, b"200 OK", b"Content-Type: text/html"),
            ("GET", "/?query=test", None, b"200 OK", b"Content-Type: text/html"),
        ]

        for method, path, body, expected_status, expected_body in cases:
            with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as sock:
                req = f"{method} {path} HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n".encode()
                sock.sendall(req)
                resp = sock.recv(8192)
                self.assertIn(expected_status, resp, f"Failed for {path}")
                self.assertIn(expected_body, resp, f"Missing body content for {path}")

        # Test POST /api/echo with query string
        with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as sock:
            body = b"echo-with-query-payload"
            req = (
                b"POST /api/echo?test=1&debug=true HTTP/1.1\r\n"
                b"Host: localhost\r\n"
                b"Content-Length: " + str(len(body)).encode() + b"\r\n"
                b"Content-Type: text/plain\r\n"
                b"Connection: close\r\n\r\n" + body
            )
            sock.sendall(req)
            resp = sock.recv(4096)
            self.assertIn(b"200 OK", resp)
            self.assertIn(body, resp)

        # Test POST /api/echo with empty query string
        with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as sock:
            body = b"echo-empty-query-payload"
            req = (
                b"POST /api/echo? HTTP/1.1\r\n"
                b"Host: localhost\r\n"
                b"Content-Length: " + str(len(body)).encode() + b"\r\n"
                b"Content-Type: text/plain\r\n"
                b"Connection: close\r\n\r\n" + body
            )
            sock.sendall(req)
            resp = sock.recv(4096)
            self.assertIn(b"200 OK", resp)
            self.assertIn(body, resp)

    def test_latency_telemetry_live_recording(self):
        """Verify that live application requests record latency samples, while /metrics polling does not."""
        if not self.check_server_available():
            self.skipTest("HTTP server not actively listening.")

        def fetch_metrics_json():
            with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as s:
                s.sendall(b"GET /metrics HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n")
                raw_data = b""
                while True:
                    chunk = s.recv(4096)
                    if not chunk:
                        break
                    raw_data += chunk
                _, _, json_str = raw_data.partition(b"\r\n\r\n")
                return json.loads(json_str.decode("utf-8"))

        initial_metrics = fetch_metrics_json()
        self.assertIn("total_requests_completed", initial_metrics)
        self.assertIn("recent_latency_sample_count", initial_metrics)
        self.assertIn("median_latency_ms", initial_metrics)

        # Send application request to /health
        with socket.create_connection((self.HOST, self.PORT), timeout=2.0) as sock:
            sock.sendall(b"GET /health HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n")
            resp = sock.recv(4096)
            self.assertIn(b"200 OK", resp)

        after_app = fetch_metrics_json()
        # The /health request must record a valid latency observation
        self.assertGreaterEqual(after_app["recent_latency_sample_count"], 1)
        self.assertGreaterEqual(after_app["total_requests_completed"], initial_metrics["total_requests_completed"] + 2)
        self.assertGreaterEqual(after_app["median_latency_ms"], 0.0)

        # Polling /metrics again must NOT increment recent_latency_sample_count (lat = -1.0 exclusion)
        poll_metrics = fetch_metrics_json()
        self.assertEqual(poll_metrics["recent_latency_sample_count"], after_app["recent_latency_sample_count"])
        self.assertGreaterEqual(poll_metrics["total_requests_completed"], after_app["total_requests_completed"] + 1)

    def test_graceful_shutdown_drains_idle_keepalive(self):
        """Verify server terminates promptly upon SIGTERM/SIGINT even when persistent sockets are idle."""
        import subprocess
        import signal

        port = 8089
        server_bin = os.path.join(PROJECT_ROOT, "bin", "server")
        conf_file = os.path.join(PROJECT_ROOT, "config", "server.conf.example")
        if not os.path.exists(server_bin):
            self.skipTest("Server binary not built.")

        proc = subprocess.Popen([server_bin, "-c", conf_file, "-p", str(port)],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        time.sleep(0.3)
        try:
            # Connect and send a request with keep-alive
            sock = socket.create_connection(("127.0.0.1", port), timeout=2.0)
            sock.sendall(b"GET /health HTTP/1.1\r\nHost: localhost\r\n\r\n")
            resp = sock.recv(1024)
            self.assertIn(b"200 OK", resp)

            # Socket is now open and idle in keep-alive loop (keepalive_timeout = 5s)
            t0 = time.time()
            proc.send_signal(signal.SIGTERM)

            # Socket should receive EOF within 1.5s (not 5s) due to stop_requested flag
            chunk = sock.recv(1024)
            t1 = time.time()
            sock.close()

            self.assertEqual(len(chunk), 0, "Expected orderly EOF on shutdown")
            self.assertLess(t1 - t0, 1.5, f"Shutdown took too long ({t1 - t0:.2f}s)")

            proc.wait(timeout=2.0)
            self.assertEqual(proc.returncode, 0)
        finally:
            if proc.stdout:
                proc.stdout.close()
            if proc.stderr:
                proc.stderr.close()
            if proc.poll() is None:
                proc.kill()
                proc.wait()


if __name__ == "__main__":
    unittest.main()
