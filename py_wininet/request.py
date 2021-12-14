import io
from urllib.request import HTTPHandler, HTTPSHandler, ProxyHandler, Request
from http.client import HTTPResponse

import _py_wininet

from _py_wininet import Session, Stream


class WininetHandler(HTTPHandler, HTTPSHandler, ProxyHandler):
    class Wrapper(io.RawIOBase):
        def __init__(self, stream: Stream):
            self.stream = stream

        def readinto(self, buffer) -> int:
            return self.stream.readinto(buffer)

        def readable(self) -> bool:
            return True

        def makefile(self, _mode, buffering=8192, *_args, **_kwargs):
            return io.BufferedReader(self, buffer_size=buffering)

    # noinspection PyMissingConstructor
    def __init__(self, agent: str = None, proxy: str = None, proxypass: str = None):
        kwargs = dict()
        if agent:
            kwargs['agent'] = agent
        if proxy:
            kwargs['proxy'] = proxy
        if proxypass:
            kwargs['proxypass'] = proxypass
        self.session = Session(**kwargs)

    # noinspection PyTypeChecker,PyUnresolvedReferences
    def http_open(self, req: Request) -> HTTPResponse:
        req = self.do_request_(req)
        headers = ''.join('{}: {}\r\n'.format(k, v) for k, v in req.headers.items())
        headers += ''.join('{}: {}\r\n'.format(k, v) for k, v in req.unredirected_hdrs.items())
        kwargs = {'headers': headers + '\r\n'} if len(headers) > 0 else {}
        split = req.host.split(':', 1)
        if len(split) == 1:
            host = req.host
        else:
            host = split[0]
            kwargs['port'] = int(split[1])
        if req.data:
            kwargs['data'] = req.data
        if req.origin_req_host and (req.origin_req_host != host):
            kwargs['referer'] = req.origin_req_host
        if req.has_header('accept-types'):
            kwargs['accept-types'] = req.get_header('accept-types')
            req.remove_header('accept-types')
        stream = _py_wininet.Stream(self.session.handle, req.type, host,
                                    req.get_method(), req.selector,
                                    req.full_url, **kwargs)
        r = HTTPResponse(self.Wrapper(stream))
        r.begin()
        r.msg = r.reason
        r.url = stream.url
        r.chunked = False
        return r
