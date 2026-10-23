"""Logging for the add-on: every module logs through here to one file.

The log is sbx_parts.log in the add-on's user folder (shown in the
add-on's preferences), kept to two old copies of 2 MB. Find problems
with it: what was read, what was made, what was skipped and why.

    from .log import get_logger
    log = get_logger(__name__)
    log.info('read %s: %d shapes', name, count)
"""

import logging
import logging.handlers
import os
import tempfile

LOG_NAME = 'sbx_parts.log'
_ROOT = 'sbx_parts'
_handler = None


def log_folder():
    try:
        import bpy
        return bpy.utils.extension_path_user(__package__, path='logs', create=True)
    except Exception:  # not running as an installed extension (tests)
        path = os.path.join(tempfile.gettempdir(), 'sbx_parts_logs')
        os.makedirs(path, exist_ok=True)
        return path


def log_path():
    return os.path.join(log_folder(), LOG_NAME)


def setup():
    """Attach the file handler (once). Called at registration."""
    global _handler
    root = logging.getLogger(_ROOT)
    root.setLevel(logging.DEBUG)
    if _handler is None:
        _handler = logging.handlers.RotatingFileHandler(log_path(), maxBytes=2 * 1024 * 1024,
                                                        backupCount=2, encoding='utf-8')
        _handler.setFormatter(logging.Formatter('%(asctime)s %(levelname)-7s %(name)s: %(message)s'))
        root.addHandler(_handler)
    root.info('---- SBX Part Tools loaded; log: %s', log_path())


def teardown():
    global _handler
    if _handler is not None:
        logging.getLogger(_ROOT).removeHandler(_handler)
        _handler.close()
        _handler = None


def get_logger(name):
    """A logger under the add-on's root ('sbx_parts.import_part' ...)."""
    return logging.getLogger(_ROOT + '.' + name.rsplit('.', 1)[-1])
