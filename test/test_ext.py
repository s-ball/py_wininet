import unittest
from .echo_server import EchoServer
import _py_wininet
from urllib.request import build_opener, Request, HTTPHandler


class TestHttp(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.opener = build_opener()
        for h in cls.opener.handlers:
            if isinstance(h, HTTPHandler):
                cls.h = h
        cls.serv = EchoServer('localhost', 8080)
        cls.serv.start()

    def setUp(self) -> None:
        self.session = _py_wininet.Session()

    def tearDown(self) -> None:
        self.session.close()

    @classmethod
    def tearDownClass(cls) -> None:
        cls.serv.shutdown()

    def testFoo(self):
        url = 'http://localhost:8080/foo'
        req = Request(url)
        decomp = req.host.split(':', 1)
        if len(decomp) == 2:
            host, port = decomp
        else:
            host, port = req.host, 80
        stream = _py_wininet.Stream(self.session.handle, req.type, host,
                                    req.get_method(), req.selector,
                                    req.full_url, port=int(port),
                                    headers='Host: ' + host +'\r\n\r\n')
        b = bytearray(2048)
        l = stream.readinto(b)
        self.assertTrue(l > 0)
        l = stream.readinto(b)
        self.assertTrue(l > b)
        self.assertTrue(l < len(b))


if __name__ == '__main__':
    unittest.main()
