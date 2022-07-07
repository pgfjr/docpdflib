# docpdflib

This is a C++ library for generating PDF documents. The API is largely based on the PostScript imaging model (see examples.cpp).

This is a work in progress; error checking is very basic as most methods simply return a Boolean value. To obtain the specific error, call the method 'get_error_type'. Error messages are usually generated only when an exception is thrown; so the method 'get_error_message' may or may not contain any useful information.

Currently, only Type 1 fonts are supported. The included fonts are from URW. Only the 14 base fonts are included in this repository. For the complete sent of fonts, please check this link: https://github.com/ArtifexSoftware/urw-base35-fonts/tree/master/fonts. These fonts must be stored in a folder named 'fonts', which must be located where your executable is.
