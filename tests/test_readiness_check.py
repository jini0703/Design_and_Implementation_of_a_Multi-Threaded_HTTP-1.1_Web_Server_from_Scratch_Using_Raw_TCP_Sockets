#!/usr/bin/env python3
"""
test_readiness_check.py
Unit and integration tests for benchmark/run_reproducible_evaluation.py:wait_for_server().

Verifies:
  1. The actual server is ready and returns the expected health JSON.
  2. A port is open, but the response comes from an unrelated HTTP service or non-HTTP listener.
  3. The health endpoint returns a non-200 status (e.g. 503, 500, 404).
  4. The health endpoint returns malformed or unexpected JSON (invalid syntax, arrays, missing fields).
  5. The expected server is not running and the polling budget expires (distinguishing per-operation socket timeouts from an end-to-end deadline).
  6. Transient retry: server transitions from not ready to ready before deadline.
  7. Live server integration: verifies against the real compiled C server binary if present.

All mock tests run on ephemeral ports (port 0) to avoid depending on or conflicting with port 8080.
"""

import http.server
import json
import os
import signal
import socket
import subprocess
import sys
import threading
import time
import unittest

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
if PROJECT_ROOT not in sys.path:
    sys.path.insert(0, PROJECT_ROOT)

from benchmark.run_reproducible_evaluation import wait_for_server


class MockHttpServer:
    """Configurable isolated mock HTTP server on an ephemeral loopback port."""

    def __init__(self, response_callback):
        self.response_callback = response_callback
        self.port = None
        self.request_count = 0
        self._server = None
        self._thread = None

    def start(self):
        parent = self

        class Handler(http.server.BaseHTTPRequestHandler):
            def log_message(self, format, *args):
                # Suppress noisy logging during unit tests
                pass

            def do_GET(self):
                parent.request_count += 1
                status, headers, body = parent.response_callback(parent.request_count, self.path)
                self.send_response(status)
                for k, v in headers.items():
                    self.send_header(k, v)
                self.end_headers()
                if body:
                    self.wfile.write(body)

        self._server = http.server.HTTPServer(("127.0.0.1", 0), Handler)
        self.port = self._server.server_address[1]
        self._thread = threading.Thread(target=self._server.serve_forever, daemon=True)
        self._thread.start()
        return self

    def stop(self):
        if self._server:
            self._server.shutdown()
            self._server.server_close()
        if self._thread:
            self._thread.join(timeout=1.0)


class MockRawTcpServer:
    """Isolated raw TCP server on an ephemeral loopback port to test non-HTTP listeners."""

    def __init__(self, responder_func):
        self.responder_func = responder_func
        self.port = None
        self._sock = None
        self._thread = None
        self._stop = threading.Event()

    def start(self):
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._sock.bind(("127.0.0.1", 0))
        self.port = self._sock.getsockname()[1]
        self._sock.listen(5)
        self._sock.settimeout(0.5)

        def run():
            while not self._stop.is_set():
                try:
                    conn, _ = self._sock.accept()
                except socket.timeout:
                    continue
                except OSError:
                    break
                try:
                    self.responder_func(conn)
                except Exception:
                    pass
                finally:
                    try:
                        conn.close()
                    except Exception:
                        pass

        self._thread = threading.Thread(target=run, daemon=True)
        self._thread.start()
        return self

    def stop(self):
        self._stop.set()
        if self._sock:
            try:
                self._sock.close()
            except Exception:
                pass
        if self._thread:
            self._thread.join(timeout=1.0)


class TestReadinessCheck(unittest.TestCase):

    def test_case1_expected_server_ready(self):
        """Case 1: The actual server is ready and returns the expected health JSON."""
        expected_body = json.dumps({
            "status": "healthy",
            "server": "HTTP-Server-C17/1.0.0",
            "protocol": "HTTP/1.1",
        }).encode("utf-8")

        def cb(count, path):
            self.assertEqual(path, "/health")
            return (200, {"Content-Type": "application/json; charset=utf-8"}, expected_body)

        server = MockHttpServer(cb).start()
        try:
            self.assertTrue(wait_for_server("127.0.0.1", server.port, max_wait_sec=2.0))
            self.assertGreaterEqual(server.request_count, 1)
        finally:
            server.stop()

    def test_case2a_unrelated_http_service_rejected(self):
        """Case 2a: A port is open, but the response comes from an unrelated HTTP service (e.g. nginx)."""
        nginx_body = json.dumps({
            "server": "nginx/1.24.0",
            "protocol": "HTTP/1.1",
            "status": "ok",
        }).encode("utf-8")

        def cb(count, path):
            return (200, {"Content-Type": "application/json"}, nginx_body)

        server = MockHttpServer(cb).start()
        try:
            # Must reject unrelated server and return False after timeout
            self.assertFalse(wait_for_server("127.0.0.1", server.port, max_wait_sec=0.2))
        finally:
            server.stop()

    def test_case2b_unrelated_html_service_rejected(self):
        """Case 2b: A port is open, but returns an HTML page (e.g. Apache default)."""
        html_body = b"<!DOCTYPE html><html><body><h1>Apache2 Default Page</h1></body></html>"

        def cb(count, path):
            return (200, {"Content-Type": "text/html"}, html_body)

        server = MockHttpServer(cb).start()
        try:
            self.assertFalse(wait_for_server("127.0.0.1", server.port, max_wait_sec=0.2))
        finally:
            server.stop()

    def test_case2c_non_http_listener_rejected(self):
        """Case 2c: A port is open, but speaks a non-HTTP protocol (e.g. SSH banner)."""
        def responder(conn):
            conn.recv(1024)
            conn.sendall(b"SSH-2.0-OpenSSH_8.9p1 Ubuntu-3ubuntu0.7\r\n")

        raw_server = MockRawTcpServer(responder).start()
        try:
            self.assertFalse(wait_for_server("127.0.0.1", raw_server.port, max_wait_sec=0.2))
        finally:
            raw_server.stop()

    def test_case3_non_200_status_rejected(self):
        """Case 3: The health endpoint returns non-200 statuses (503, 500, 404)."""
        for code in [503, 500, 404]:
            with self.subTest(status_code=code):
                error_body = json.dumps({
                    "status": "unhealthy",
                    "server": "HTTP-Server-C17/1.0.0",
                    "protocol": "HTTP/1.1",
                }).encode("utf-8")

                def cb(count, path):
                    return (code, {"Content-Type": "application/json"}, error_body)

                server = MockHttpServer(cb).start()
                try:
                    self.assertFalse(wait_for_server("127.0.0.1", server.port, max_wait_sec=0.2))
                finally:
                    server.stop()

    def test_case4a_malformed_json_rejected(self):
        """Case 4a: The health endpoint returns 200 OK with malformed JSON syntax."""
        def cb(count, path):
            return (200, {"Content-Type": "application/json"}, b"{status: broken, server: 'unquoted}")

        server = MockHttpServer(cb).start()
        try:
            self.assertFalse(wait_for_server("127.0.0.1", server.port, max_wait_sec=0.2))
        finally:
            server.stop()

    def test_case4b_unexpected_json_structure_rejected(self):
        """Case 4b: The health endpoint returns 200 OK with unexpected JSON structure (array or missing keys)."""
        bad_payloads = [
            # JSON array instead of dict
            json.dumps(["HTTP-Server-C17/1.0.0", "HTTP/1.1"]).encode("utf-8"),
            # Missing protocol
            json.dumps({"status": "healthy", "server": "HTTP-Server-C17/1.0.0"}).encode("utf-8"),
            # Missing server
            json.dumps({"status": "healthy", "protocol": "HTTP/1.1"}).encode("utf-8"),
            # Missing status
            json.dumps({"server": "HTTP-Server-C17/1.0.0", "protocol": "HTTP/1.1"}).encode("utf-8"),
            # Unexpected types
            json.dumps({"status": 123, "server": ["not-a-string"], "protocol": True}).encode("utf-8"),
            # Empty object
            json.dumps({}).encode("utf-8"),
        ]

        for payload in bad_payloads:
            with self.subTest(payload=payload):
                def cb(count, path):
                    return (200, {"Content-Type": "application/json"}, payload)

                server = MockHttpServer(cb).start()
                try:
                    self.assertFalse(wait_for_server("127.0.0.1", server.port, max_wait_sec=0.15))
                finally:
                    server.stop()

    def test_case4c_missing_status_field_rejected(self):
        """Case 4c: Response has the expected server identity and protocol but omits 'status'."""
        payload = json.dumps({
            "server": "HTTP-Server-C17/1.0.0",
            "protocol": "HTTP/1.1",
        }).encode("utf-8")

        def cb(count, path):
            return (200, {"Content-Type": "application/json"}, payload)

        server = MockHttpServer(cb).start()
        try:
            self.assertFalse(
                wait_for_server("127.0.0.1", server.port, max_wait_sec=0.15),
                "wait_for_server must reject response when 'status' is omitted",
            )
        finally:
            server.stop()

    def test_case5a_server_unavailable_deadline_respected(self):
        """Case 5a: The server is unavailable (connection refused); polling loop respects max_wait_sec budget."""
        # Find a confirmed unused ephemeral port by binding and immediately closing
        temp_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        temp_sock.bind(("127.0.0.1", 0))
        free_port = temp_sock.getsockname()[1]
        temp_sock.close()

        t_start = time.monotonic()
        result = wait_for_server("127.0.0.1", free_port, max_wait_sec=0.15)
        t_elapsed = time.monotonic() - t_start

        self.assertFalse(result, "Unreachable port must return False")
        self.assertGreaterEqual(t_elapsed, 0.14, "Must not return before configured wait deadline")
        self.assertLess(t_elapsed, 0.35, "Cooperative polling with per-operation timeout must not excessively overshoot budget")

    def test_case5b_slow_server_deadline_respected(self):
        """Case 5b: Server accepts connection but hangs; per-operation socket timeout is bounded by remaining budget."""
        def responder(conn):
            # Read request, but do not send response within the test's deadline
            try:
                conn.recv(1024)
            except Exception:
                pass
            time.sleep(0.4)

        raw_server = MockRawTcpServer(responder).start()
        try:
            t_start = time.monotonic()
            result = wait_for_server("127.0.0.1", raw_server.port, max_wait_sec=0.15)
            t_elapsed = time.monotonic() - t_start

            self.assertFalse(result, "Slow/hanging server must return False after deadline")
            self.assertGreaterEqual(t_elapsed, 0.14, "Must wait up to deadline budget")
            self.assertLess(t_elapsed, 0.35, "Per-operation socket timeout must not exceed remaining budget")
        finally:
            raw_server.stop()

    def test_case6_transient_retry_until_ready(self):
        """Case 6: Server is temporarily unavailable (e.g. 503) on first attempts, then becomes ready."""
        healthy_body = json.dumps({
            "status": "healthy",
            "server": "HTTP-Server-C17/1.0.0",
            "protocol": "HTTP/1.1",
        }).encode("utf-8")

        def cb(count, path):
            if count <= 2:
                # Still initializing / warming up
                return (503, {"Content-Type": "application/json"}, b'{"status": "starting"}')
            return (200, {"Content-Type": "application/json"}, healthy_body)

        server = MockHttpServer(cb).start()
        try:
            self.assertTrue(wait_for_server("127.0.0.1", server.port, max_wait_sec=2.0))
            self.assertGreaterEqual(server.request_count, 3)
        finally:
            server.stop()

    def test_case7_live_c17_server_readiness_probe(self):
        """Case 7: Integration check against the live compiled C server binary on an ephemeral port."""
        server_bin = os.path.join(PROJECT_ROOT, "bin", "server")
        conf_file = os.path.join(PROJECT_ROOT, "config", "server.conf.example")

        if not os.path.exists(server_bin) or not os.path.exists(conf_file):
            self.skipTest("C server binary not compiled; run 'make' first")

        # Pick an ephemeral port for isolated test execution
        temp_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        temp_sock.bind(("127.0.0.1", 0))
        test_port = temp_sock.getsockname()[1]
        temp_sock.close()

        proc = subprocess.Popen(
            [server_bin, "-c", conf_file, "-p", str(test_port)],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        try:
            self.assertTrue(
                wait_for_server("127.0.0.1", test_port, max_wait_sec=5.0),
                f"wait_for_server failed to verify live server on port {test_port}",
            )
        finally:
            try:
                proc.terminate()
                proc.wait(timeout=2.0)
            except Exception:
                proc.kill()


if __name__ == "__main__":
    unittest.main()
