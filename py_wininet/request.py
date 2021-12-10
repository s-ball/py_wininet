import io
from urllib.request import HTTPHandler, HTTPSHandler, Request
from http.client import HTTPResponse, HTTPConnection


class WininetHandler(HTTPHandler, HTTPSHandler):
    class DummyStream(io.RawIOBase):
        data: bytes = b'''HTTP/1.1 200 OK\r
Content-Type: text/plain\r
Content-Length: 18\r
\r
abc\r
defg\r
hijkl\r
'''

        def __init__(self, url: str):
            self.fd = io.BytesIO(self.data)
            self.url = url

        def readinto(self, _buffer) -> int:
            return self.fd.readinto(_buffer)

        def makefile(self, _mode, buffering=8192, *_args, **_kwargs):
            # noinspection PyTypeChecker
            return io.BufferedReader(self.fd, buffer_size=buffering)

    def http_open(self, req: Request) -> HTTPResponse:
        print(type(req), req)
        # noinspection PyTypeChecker
        stream = self.DummyStream(req.full_url)
        r = HTTPResponse(stream)
        r.begin()
        r.msg = r.reason
        r.url = stream.url
        return r
