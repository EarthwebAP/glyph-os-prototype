"""
Setup script for SPU merge Python bindings

Build: python3 setup.py build_ext --inplace
"""

import sys
from pathlib import Path

try:
    from pybind11.setup_helpers import Pybind11Extension, build_ext
    from setuptools import setup

    ext_modules = [
        Pybind11Extension(
            "spu_merge",
            sources=["bindings.cpp", "merge_ref.cpp"],
            include_dirs=["."],
            extra_compile_args=["-O3", "-std=c++17"],
        ),
    ]

    setup(
        name="spu_merge",
        version="1.0.0",
        author="Glyph OS",
        description="SPU merge primitive - C++ accelerated glyph merging",
        ext_modules=ext_modules,
        cmdclass={"build_ext": build_ext},
        zip_safe=False,
        python_requires=">=3.7",
    )

except ImportError:
    print("ERROR: pybind11 not found!", file=sys.stderr)
    print("", file=sys.stderr)
    print("Install with:", file=sys.stderr)
    print("  pip install pybind11", file=sys.stderr)
    print("", file=sys.stderr)
    print("Or use the ctypes binding instead:", file=sys.stderr)
    print("  python3 -c 'import spu_ctypes; spu_ctypes.test()'", file=sys.stderr)
    sys.exit(1)
