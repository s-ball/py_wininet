import unittest
from _py_wininet import Session


class TestSession(unittest.TestCase):
    def test_new(self):
        sess = Session()
        self.assertFalse(sess.closed)
        self.assertNotEqual(0, sess.handle)

    def test_closed(self):
        sess = Session()
        sess.close()
        self.assertEqual(0, sess.handle)
        self.assertTrue(sess.closed)


if __name__ == '__main__':
    unittest.main()
