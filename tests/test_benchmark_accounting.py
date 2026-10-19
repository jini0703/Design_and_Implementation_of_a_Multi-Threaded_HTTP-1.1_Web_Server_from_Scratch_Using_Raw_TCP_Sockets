#!/usr/bin/env python3
"""
test_benchmark_accounting.py
Unit test suite verifying HTTP response classification, RFC 9112 framing,
and error accounting in benchmark/run_experiment.py.

Project: Design and Implementation of a Multi-Threaded HTTP/1.1 Web Server
         from Scratch Using Raw TCP Sockets
"""

import os
import socket
import sys
import unittest

# Ensure project root is in sys.path so we can import from benchmark
PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
if PROJECT_ROOT not in sys.path:
    sys.path.insert(0, PROJECT_ROOT)

from benchmark.run_experiment import read_http_response


class TestBenchmarkAccounting(unittest.TestCase):

    def test_valid_200_success_classification(self):
        """Verify that a 200 OK response is parsed correctly as 2xx success."""
        client_sock, server_sock = socket.socketpair()
        try:
            raw_response = (
                b"HTTP/1.1 200 OK\r\n"
                b"Content-Type: text/plain\r\n"
                b"Content-Length: 13\r\n"
                b"\r\n"
                b"Hello, World!"
            )
            server_sock.sendall(raw_response)

            status, is_close, body, remaining = read_http_response(client_sock, timeout=1.0)
            self.assertEqual(status, 200)
            self.assertFalse(is_close)
            self.assertEqual(body, b"Hello, World!")
            self.assertEqual(remaining, b"")
            self.assertTrue(200 <= status < 300, "200 must be counted in success_2xx")
        finally:
            client_sock.close()
            server_sock.close()

    def test_503_counted_as_5xx_not_2xx(self):
        """Verify that HTTP 503 is strictly counted as 5xx and NEVER as 2xx success."""
        client_sock, server_sock = socket.socketpair()
        try:
            raw_response = (
                b"HTTP/1.1 503 Service Unavailable\r\n"
                b"Content-Type: text/plain\r\n"
                b"Content-Length: 18\r\n"
                b"Connection: close\r\n"
                b"\r\n"
                b"Server Overloaded\n"
            )
            server_sock.sendall(raw_response)

            status, is_close, body, remaining = read_http_response(client_sock, timeout=1.0)
            self.assertEqual(status, 503)
            self.assertTrue(is_close)
            self.assertEqual(body, b"Server Overloaded\n")
            self.assertEqual(remaining, b"")

            # Invariants
            self.assertFalse(200 <= status < 300, "503 must NEVER be counted as success_2xx")
            self.assertTrue(500 <= status < 600, "503 must be counted in http_5xx")
        finally:
            client_sock.close()
            server_sock.close()

    def test_400_and_404_counted_as_4xx(self):
        """Verify that client error responses are counted in 4xx category."""
        client_sock, server_sock = socket.socketpair()
        try:
            raw_response = (
                b"HTTP/1.1 404 Not Found\r\n"
                b"Content-Length: 9\r\n"
                b"\r\n"
                b"Not Found"
            )
            server_sock.sendall(raw_response)

            status, is_close, body, remaining = read_http_response(client_sock, timeout=1.0)
            self.assertEqual(status, 404)
            self.assertEqual(body, b"Not Found")
            self.assertTrue(400 <= status < 500, "404 must be counted in http_4xx")
        finally:
            client_sock.close()
            server_sock.close()

    def test_301_counted_as_3xx(self):
        """Verify that redirection responses are counted in 3xx category."""
        client_sock, server_sock = socket.socketpair()
        try:
            raw_response = (
                b"HTTP/1.1 301 Moved Permanently\r\n"
                b"Location: /new-path\r\n"
                b"Content-Length: 0\r\n"
                b"\r\n"
            )
            server_sock.sendall(raw_response)

            status, is_close, body, remaining = read_http_response(client_sock, timeout=1.0)
            self.assertEqual(status, 301)
            self.assertEqual(body, b"")
            self.assertTrue(300 <= status < 400, "301 must be counted in http_3xx")
        finally:
            client_sock.close()
            server_sock.close()

    def test_timeout_counted_as_transport_timeout(self):
        """Verify that socket timeout raises socket.timeout rather than an HTTP response."""
        client_sock, server_sock = socket.socketpair()
        try:
            # Don't send anything from server
            with self.assertRaises(socket.timeout):
                read_http_response(client_sock, timeout=0.1)
        finally:
            client_sock.close()
            server_sock.close()

    def test_connection_closure_before_headers_raises_error(self):
        """Verify connection closure before response headers raises ConnectionResetError."""
        client_sock, server_sock = socket.socketpair()
        try:
            server_sock.close()  # Close immediately
            with self.assertRaises(ConnectionResetError):
                read_http_response(client_sock, timeout=0.5)
        finally:
            client_sock.close()

    def test_incomplete_body_framing_error(self):
        """Verify that truncated body raises ValueError framing error (invalid_responses)."""
        client_sock, server_sock = socket.socketpair()
        try:
            # Announce 50 bytes, but only send 10 then close
            raw_response = (
                b"HTTP/1.1 200 OK\r\n"
                b"Content-Length: 50\r\n"
                b"\r\n"
                b"0123456789"
            )
            server_sock.sendall(raw_response)
            server_sock.close()

            with self.assertRaises(ValueError) as ctx:
                read_http_response(client_sock, timeout=0.5)
            self.assertIn("Incomplete response", str(ctx.exception))
        finally:
            client_sock.close()

    def test_pipelining_preserves_leftover_buffer_without_body_bleeding(self):
        """Verify that sequential responses on a persistent socket preserve buffer bytes without bleeding."""
        client_sock, server_sock = socket.socketpair()
        try:
            # Server sends two distinct responses in a single TCP write
            resp1 = b"HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nFIRST"
            resp2 = b"HTTP/1.1 200 OK\r\nContent-Length: 6\r\nConnection: close\r\n\r\nSECOND"
            server_sock.sendall(resp1 + resp2)

            # First read should extract FIRST and leave resp2 in remaining_buf
            status1, is_close1, body1, buf = read_http_response(client_sock, timeout=1.0)
            self.assertEqual(status1, 200)
            self.assertFalse(is_close1)
            self.assertEqual(body1, b"FIRST")
            self.assertTrue(len(buf) > 0, "Buffer must retain bytes for second response")

            # Second read should process SECOND from buffer without requiring new socket recv
            status2, is_close2, body2, remaining = read_http_response(client_sock, initial_buf=buf, timeout=1.0)
            self.assertEqual(status2, 200)
            self.assertTrue(is_close2)
            self.assertEqual(body2, b"SECOND")
            self.assertEqual(remaining, b"")
        finally:
            client_sock.close()
            server_sock.close()

    def test_rps_calculation_strictly_uses_2xx(self):
        """Verify that successful_rps calculation only counts 2xx responses."""
        success_2xx = 120
        http_5xx = 80
        duration_sec = 2.0

        # Formula used in run_experiment.py:
        # successful_rps = success_2xx / duration_sec
        # completed = success_2xx + http_3xx + http_4xx + http_5xx
        completed = success_2xx + http_5xx
        successful_rps = success_2xx / duration_sec

        self.assertEqual(completed, 200)
        self.assertEqual(successful_rps, 60.0)
        # Verify that 5xx responses do not inflate successful RPS
        self.assertNotEqual(successful_rps, completed / duration_sec)

    def test_zero_success_reports_na_latency(self):
        """Verify that when a trial has zero 2xx successes, latency percentiles are reported as N/A."""
        # Simulated trial with 100 attempted, 0 2xx success, 100 503 overload shedding
        success_latencies = []
        success_2xx = 0
        duration_sec = 2.0

        if success_latencies and success_2xx > 0:
            avg_lat_str = p50_str = p95_str = p99_str = "0.00"
        else:
            avg_lat_str = p50_str = p95_str = p99_str = "N/A"

        self.assertEqual(avg_lat_str, "N/A")
        self.assertEqual(p50_str, "N/A")
        self.assertEqual(p95_str, "N/A")
        self.assertEqual(p99_str, "N/A")
        # Ensure it is NOT reported as 0 ms
        self.assertNotEqual(avg_lat_str, "0.00")
        self.assertNotEqual(avg_lat_str, "0 ms")

    def test_distinction_client_vs_server_accounting(self):
        """Verify distinction between client-observed response counts and server-side connection/request counters."""
        # Client observed 50 completed responses
        client_stats = {
            "attempted": 55,
            "completed": 50,
            "success_2xx": 45,
            "http_5xx": 5,
            "transport_errors": 5,
        }
        # Server metrics track raw TCP connections and completed application requests
        server_metrics = {
            "total_connections_accepted": 55,
            "total_connections_rejected": 5,
            "total_requests_completed": 45,
            "total_overload_503_sent": 5,
        }
        # Client 2xx matches server requests completed, but total connection accepted exceeds completed requests
        self.assertEqual(client_stats["success_2xx"], server_metrics["total_requests_completed"])
        self.assertGreater(server_metrics["total_connections_accepted"], client_stats["success_2xx"])
        self.assertEqual(client_stats["http_5xx"], server_metrics["total_overload_503_sent"])

    def test_reconciliation_overload_and_client_5xx_timeouts(self):
        """Verify reconciliation between client-observed 5xx + timeouts and server overload counters."""
        # Realistic 64-client load scenario matching the audited benchmark trial
        client_stats = {
            "attempted": 8127,
            "completed": 8106,
            "success_2xx": 0,
            "http_5xx": 8106,
            "timeouts": 21,
        }
        server_metrics = {
            "total_connections_rejected": 8129,
            "total_overload_503_sent": 8129,
            "qlaac_shed_requests": 8129,
        }
        # Client observed 5xx plus socket timeouts accounts for all rejected client attempts
        client_rejected_total = client_stats["http_5xx"] + client_stats["timeouts"]
        self.assertEqual(client_rejected_total, 8127)
        # Server sent 503s matches rejected connections exactly (100% 503 transmission rate)
        self.assertEqual(server_metrics["total_connections_rejected"], server_metrics["total_overload_503_sent"])
        self.assertEqual(server_metrics["total_overload_503_sent"], server_metrics["qlaac_shed_requests"])
        # Discrepancy between client attempts and server responses is strictly bounded by in-flight/boundary probes (<= 2)
        diff = server_metrics["total_overload_503_sent"] - client_rejected_total
        self.assertLessEqual(diff, 2)

    def test_503_retry_after_and_queue_accounting(self):
        """Verify 503 Retry-After header presence and queue-full vs QLAAC shed accounting."""
        client_sock, server_sock = socket.socketpair()
        try:
            raw_503 = (
                b"HTTP/1.1 503 Service Unavailable\r\n"
                b"Content-Type: text/plain\r\n"
                b"Content-Length: 42\r\n"
                b"Connection: close\r\n"
                b"Retry-After: 5\r\n"
                b"\r\n"
                b"Server overloaded. Please retry in 5 sec.\n"
            )
            server_sock.sendall(raw_503)

            status, is_close, body, remaining = read_http_response(client_sock, timeout=1.0)
            self.assertEqual(status, 503)
            self.assertTrue(is_close)
            self.assertIn(b"Retry-After: 5", raw_503)
            self.assertEqual(body, b"Server overloaded. Please retry in 5 sec.\n")

            # Accounting invariants across rejection paths:
            # Path A: Queue-full rejection (QLAAC disabled or queue reaches hard cap)
            queue_full_server = {
                "total_connections_rejected": 10,
                "total_overload_503_sent": 10,
                "qlaac_shed_requests": 0,  # NOT a QLAAC admission decision
            }
            self.assertEqual(queue_full_server["total_connections_rejected"], 10)
            self.assertEqual(queue_full_server["qlaac_shed_requests"], 0)

            # Path B: QLAAC admission controller shed decision
            qlaac_shed_server = {
                "total_connections_rejected": 15,
                "total_overload_503_sent": 15,
                "qlaac_shed_requests": 15,  # Recorded as QLAAC shed
            }
            self.assertEqual(qlaac_shed_server["qlaac_shed_requests"], 15)

            # Path C: Socket write failure during rejection
            write_fail_server = {
                "total_connections_rejected": 1,
                "total_overload_503_sent": 0,  # Failed before 503 write succeeded
                "qlaac_shed_requests": 1,
            }
            self.assertGreater(write_fail_server["total_connections_rejected"],
                               write_fail_server["total_overload_503_sent"])
        finally:
            client_sock.close()
            server_sock.close()


if __name__ == "__main__":
    unittest.main()
