from setuptools import setup, Extension
from pkg_resources import parse_version, get_distribution, ResolutionError
import os.path
import re

from io import open

NAME = "py_wininet"

# Base version (removes any pre, post, a, b or rc element)
BASE = "0.0.0"
try:
    BASE = get_distribution(NAME).parsed_version.base_version
except (ResolutionError, OSError):
    # Try to read from version.py file
    try:
        import ast

        rx = re.compile(r'.*version.*=\s*(.*)')
        with open(os.path.join(NAME, "version.py")) as fd:
            for line in fd:
                m = rx.match(line)
                if m:
                    BASE = parse_version(ast.literal_eval(m.group(1))).base_version
                    break
    except OSError:
        pass  # give up here and stick to 0.0.0

# In long description, replace "master" in the build status badges
#  with the current version we are building
with open("README.md") as fd:
    long_description = next(fd).replace("master", BASE)
    long_description += "".join(fd)

setup(
    long_description=long_description,
    long_description_content_type="text/markdown",
    ext_modules=[
        Extension('_py_wininet',
                  ['_py_wininet/_py_wininet.c', '_py_wininet/stream.c',
                   '_py_wininet/session.c'],
                  include_dirs=['_py_wininet'],
                  libraries=['wininet'],
                  define_macros=[('UNICODE', None),
                                 ('_UNICODE', None)],
                  py_limited_api=True)
    ]
)
