from http.server import BaseHTTPRequestHandler, HTTPServer
import threading


class EchoRequestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        data = b'Request line: ' + self.requestline.encode() + b'\r\n'
        data += b'Headers:\r\n' + self.headers.as_bytes()
        length = int(self.headers.get('content-length', '0'))
        if length > 0:
            data += b'\r\nData:\r\n'
            data += self.rfile.read(length)
        self.send_response(200)
        self.send_header('Content-Length', str(len(data)))
        self.send_header('Content-type', 'text/plain')
        self.end_headers()
        self.wfile.write(data)

    def do_POST(self):
        self.do_GET()


class EchoServer(HTTPServer, threading.Thread):
    def __init__(self, host, port):
        HTTPServer.__init__(self, (host, port), EchoRequestHandler)
        threading.Thread.__init__(self)

    def run(self) -> None:
        self.serve_forever()
