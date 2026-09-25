#!/usr/bin/env python3
"""
Tiny dev server for the Arraylist preview page.

Serves the repository root (so the page can use the real fonts from
resources/fonts) and redirects "/" to the preview itself, which makes the
Arena live preview open the right page straight away.

    python3 tools/serve_preview.py [port]
"""
import http.server
import os
import socketserver
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PAGE = "/tools/arraylist-preview.html"
PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 8080


class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=ROOT, **kwargs)

    def do_GET(self):
        if self.path in ("/", "/index.html"):
            self.send_response(302)
            self.send_header("Location", PAGE)
            self.end_headers()
            return
        super().do_GET()

    def log_message(self, fmt, *args):  # keep the console quiet
        pass


class Server(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True


if __name__ == "__main__":
    with Server(("0.0.0.0", PORT), Handler) as httpd:
        print(f"Arraylist preview: http://0.0.0.0:{PORT}{PAGE}")
        httpd.serve_forever()
