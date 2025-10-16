from setuptools import setup, Extension, find_packages
import pybind11

ext_modules = [
    Extension(
        "matching_engine",
        ["cpp_engine/bindings.cpp", "cpp_engine/OrderBook.cpp","cpp_engine/market_data.cpp"],
        include_dirs=[pybind11.get_include()],
        language="c++",
    )
]

setup(
    name="crypto-matching-engine",
    version="0.1.0",
    packages=find_packages(include=["app", "app.*"]),  # Only include 'app'
    include_dirs=[pybind11.get_include(), "cpp_engine/include"],
    ext_modules=ext_modules,
)
