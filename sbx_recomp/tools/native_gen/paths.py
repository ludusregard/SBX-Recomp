"""Where the native-code tools find things. Everything is relative to the
repo this file is in, or comes from the environment:

  SBX_NATIVE_INDEX  the index index.py writes (default: idx.pkl next to
                    these tools; it's large and isn't kept in git)
  SBX_ITC_PORTED    port_native.py's --out folder (the other build's native
                    C++ at SBX's addresses), for the tools that read it
"""
import os
import pickle

HERE = os.path.dirname(os.path.abspath(__file__))
RECOMP = os.path.dirname(os.path.dirname(HERE))  # sbx_recomp/
REPO = os.path.dirname(RECOMP)
GENERATED = os.path.join(RECOMP, 'generated', 'default')
SRC = os.path.join(RECOMP, 'src')
HOOKS = os.path.join(SRC, 'hooks')
INDEX = os.environ.get('SBX_NATIVE_INDEX', os.path.join(HERE, 'idx.pkl'))
ITC_PORTED = os.environ.get('SBX_ITC_PORTED', '')


def load_index():
    """(gen, overrides, grades): each function's generated text by address,
    the source files overriding it, and port_native.py's grade for it."""
    if not os.path.exists(INDEX):
        raise SystemExit(f'no index at {INDEX}: run index.py first')
    with open(INDEX, 'rb') as f:
        return pickle.load(f)


def itc_ported():
    if not ITC_PORTED or not os.path.isdir(ITC_PORTED):
        raise SystemExit('set SBX_ITC_PORTED to port_native.py\'s --out folder')
    return ITC_PORTED
