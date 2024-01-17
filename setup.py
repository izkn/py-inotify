from setuptools import setup, Extension

setup(
    name="py-inotify",
    version="0.0.1",
    ext_modules=[
        Extension("inotify", ["src/inotifymodule.c"])
    ]
)
