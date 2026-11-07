#!/usr/bin/env python3
"""Build and test the standalone POSIX provider. Never installs or launches REAPER."""
from pathlib import Path
import hashlib
import json
import os
import platform
import shlex
import subprocess

ROOT = Path(__file__).resolve().parent
BUILD = ROOT / 'build'
ABI_SHA256 = 'e86e28b3f719119c30c1068c63a58b33a01c92b88fc560b77dd6bf287b7d8ae7'


def main():
    if platform.system() not in ('Darwin', 'Linux'):
        raise SystemExit('This minimal build supports macOS/Linux, not Windows ABI/export rules')
    if hashlib.sha256((ROOT / 'include/pitchshift_abi.h').read_bytes()).hexdigest() != ABI_SHA256:
        raise SystemExit('SDK excerpt changed; review provenance before rebuilding')
    BUILD.mkdir(exist_ok=True)
    compiler = shlex.split(os.environ.get('CXX', 'c++'))
    common = compiler + ['-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                         '-isystem', 'include']
    darwin = platform.system() == 'Darwin'
    library = BUILD / ('extrashifter.dylib' if darwin else 'extrashifter.so')
    receipts = []
    commands = [common + (['-dynamiclib'] if darwin else ['-shared', '-fPIC']) +
                ['-fvisibility=hidden', 'unity_adapter.cpp', '-o', str(library)],
                common + ['synthetic_host.cpp', '-o', str(BUILD / 'synthetic_host')] +
                ([] if darwin else ['-ldl']),
                [str(BUILD / 'synthetic_host'), str(library)]]
    for command in commands:
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=60)
        receipts.append({'command': command, 'returncode': result.returncode,
                         'stdout': result.stdout, 'stderr': result.stderr})
        (BUILD / 'test-results.json').write_text(json.dumps(receipts, indent=2) + '\n')
        print(result.stdout + result.stderr, end='')
        if result.returncode:
            raise SystemExit(result.returncode)
    print('Build and six synthetic provider test groups passed; no DAW was launched')


if __name__ == '__main__':
    main()
