# py_wininet
A python package that enable Wininet http and https accesses from
`urllib.request`.

## Current status

This package is currently in early development stage: most functionalities
are still to be implemented.

Its full source is available from
[GitHUB](https://github.com/s-ball/py_wininet).

## Goals

This module intends to allow X509 smartcard certificate authentication
over HTTPS on Windows.

Currently, the `ssl` module from the standard library requires full access to
the private key.
This is fine is that key lives on a file, but a *decent* smartcard will not
allow to export it. It instead expects to receive PKCS11 requests.

It seems that the [M2Crypto](https://gitlab.com/m2crypto/m2crypto) module
allows that, but I could not find my way with it.
Fortunately, the Wininet library on Windows does allow the use of smartcard
certificates through PKCS11. So this package will provide a `Handler` able to
process HTTP and HTTPS request, submit them to Wininet, and return a
*standard* `HTTPResponse`.

## Usage:
To be done

## Installing

**Only on Windows, because of the dependency on Wininet**
### End user installation

With pip: `pip install py_wininet`.

(not currently available)

### Developer installation

If you want to contribute or integrate `py_wininet` in your own code, you should get a copy of the full tree from [GitHUB](https://github.com/s-ball/pyimgren):

```
git clone https://github.com/s-ball/py_wininet [your_working_copy_folder]
```

#### Running the tests

As the project only uses unittest, you can simply run tests from the main folder with:

```
python -m unittest discover
```

## Contributing

As this project is developed on my free time, I cannot guarantee very fast feedbacks. Anyway, I shall be glad to receive issues or pull requests on GitHUB. 

## Versioning

This project uses a standard Major.Minor.Patch versioning pattern. Inside a major version, public API stability is expected (at least after 1.0.0 version will be published).

## License

This project is licensed under the MIT License - see the LICENSE.txt file for details