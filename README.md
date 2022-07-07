# docpdflib

This is a C++ library for generating PDF documents. The API is largely based on the PostScript imaging model (see examples.cpp).

This is a work in progress; error checking is very basic as most methods simply return a Boolean value. To obtain the specific error, call the method 'get_error_type'. Error messages are usually generated only when an exception is thrown; so the method 'get_error_message' may or may not contain any useful information.

Currently, only Type 1 fonts are supported. The included fonts are from URW. Only the 14 base fonts are included in this repository. The complete set can be found at https://ctan.org/tex-archive/fonts/urw/base35. The included fonts must be stored in a folder named 'fonts', which must be located where your executable is.

Dependencies: URW fonts, GDI+, and Zlib (64-bit DLL included). This library also uses some source codes from AGG version 2.4 (Antigrain Geometry).

Usage: Simply include the header "docpdflib.hpp" in your application and the two .cpp files from AGG (agg_bezier_arc.cpp and agg_trans_affine.cpp). See the 'examples.cpp' for more information.

See 'examples.pdf' in the 'sample folder.

<img src="samples/examples.pdf"/>
