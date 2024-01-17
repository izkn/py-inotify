# py-inotify : Lightweight bindings for [inotify(7)](https://man7.org/linux/man-pages/man7/inotify.7.html)

**py-inotify** uses the [Python/C API](https://docs.python.org/3/c-api/index.html) to create lightweight bindings
for the Linux kernel *inotify* API.

It provides functions for
[`inotify_init(2)`](https://man7.org/linux/man-pages/man2/inotify_init.2.html),
[`inotify_init1(2)`](https://man7.org/linux/man-pages/man2/inotify_init.2.html),
[`inotify_add_watch(2)`](https://man7.org/linux/man-pages/man2/inotify_add_watch.2.html), and
[`inotify_rm_watch(2)`](https://man7.org/linux/man-pages/man2/inotify_rm_watch.2.html).

In addition, it includes the function `read_events`, which reads all available events on an inotify
file descriptor and returns a list of `Event` namedtuples; one for each event. If the inotify
instance was created with the `IN_NONBLOCK` flag and its fd is empty, an empty list will be
returned.

All event constants are available at the top level of the module:

```python
>>> import inotify
>>> inotify.IN_ACCESS
1
>>> inotify.IN_ALL_EVENTS
4095
```

## Example

A basic program to monitor a directory for filesystem events:

```python
import inotify
import os

fd = inotify.init()
wd = inotify.add_watch(fd, '/example_dir', inotify.IN_ALL_EVENTS)

try:
    while True:
        for event in inotify.read_events(fd):
            print(event)
finally:
    os.close(fd)
```

Executing `touch test.txt` and `ls` in `/example_dir` after running the program produces the following:

```python
Event(wd=1, mask=256, cookie=0, len=16, name='test.txt')
Event(wd=1, mask=32, cookie=0, len=16, name='test.txt')
Event(wd=1, mask=4, cookie=0, len=16, name='test.txt')
Event(wd=1, mask=8, cookie=0, len=16, name='test.txt')
Event(wd=1, mask=1073741856, cookie=0, len=0, name='')
Event(wd=1, mask=1073741825, cookie=0, len=0, name='')
Event(wd=1, mask=1073741825, cookie=0, len=0, name='')
Event(wd=1, mask=1073741840, cookie=0, len=0, name='')
```

## Function Reference
```python
def init():
    '''Execute inotfiy_init(2).

    Initialize a new inotify instance.

    Returns:
        int: A file descriptor associated with the new inotify event queue.

    Raises:
        OSError: If the inotify_init call fails.
    '''
```
```python
def init1(flags=0):
    '''Execute inotify_init1(2).

    Initialize a new inotify instance. IN_NONBLOCK and IN_CLOEXEC can be
    bitwise ORed in flags to obtain different behavior. Use IN_NONBLOCK to
    open the file descriptor in nonblocking mode. Use IN_CLOEXEC to enable
    the close-on-exec flag for the new file descriptor.
    
    Args:
        flags (int): A bitmask for configuring file descriptor behavior.

    Returns:
        int: A file descriptor associated with the new inotify event queue.

    Raises:
        TypeError: If flags is not an integer.
        OSError: If the inotify_init1 call fails.
    '''
```
```python
def add_watch(fd, pathname, mask):
    '''Execute inotify_add_watch(2).

    Add a new watch, or modify an existing watch, for the file whose
    location is specified in pathname; the caller must have read permission
    for this file.

    Args:
        fd (int): A file descriptor referring to the inotify instance whose
            watch list is to be modified.
        pathname (str): The pathname to monitor for filesystem events.
        mask (int): Specifies the events to be monitored for pathname.

    Returns:
        int: A unique watch descriptor for the inotify instance associated
            with fd, for the filesystem object (inode) corresponding to
            pathname.

    Raises:
        TypeError: If any arguments are not of the correct type.
        OSError: If the inotify_add_watch call fails.
    '''
```
```python
def rm_watch(fd, wd):
    '''Execute inotify_rm_watch(2).

    Remove the watch associated with the watch descriptor wd from the
    inotify instance associated with the file descriptor fd.

    Args:
        fd (int): The file descriptor of the inotify instance to modify.
        wd (int): The watch descriptor of the watch to be removed.

    Returns:
        int: 0

    Raises:
        TypeError: If either fd or wd is not an integer.
        OSError: If the inotify_rm_watch call fails.
    '''
```
```python
def read_events(fd, max_bytes=0):
    '''Read and decode all events on the inotify file descriptor fd.

    If the inotify instance was initialized with IN_NONBLOCK and there
    are no events on fd, an empty tuple will be returned.

    Args:
        fd (int): An inotify file descriptor.
        max_bytes (int): The maximum number of bytes to read. This will
            result in an OSError if max_bytes is less than the size of
            one event.

    Returns:
        list: A list of Event namedtuples; one for each inotify event in
            the order they occurred.

    Raises:
        TypeError: If either fd or max_bytes is not an integer.
        OSError: If an error occurs while reading from fd.
    '''
```
