#!/usr/bin/env python3
"""Compile the patched SDK's lookup/range methods in an asset-free fixture.

Only the GPU allocation/map backend is replaced. The lookup and bitmap-reading
method bodies come directly from the SDK checkout, not copies in this test.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile


def method(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sdk', type=Path, help='SDK checkout with the project patch applied')
    parser.add_argument('--cxx', default='g++')
    args = parser.parse_args()
    source = (args.sdk / 'src/graphics/pipeline/texture/cache.cpp').read_text()
    methods = '\n'.join(method(source, signature) for signature in (
        'TextureCache::Texture* TextureCache::FindOrCreateTexture(TextureKey key)',
        'bool TextureCache::IsRangeScaledResolved(uint32_t start_unscaled, uint32_t length_unscaled)',
    ))
    fixture = Path(__file__).with_name('texture_cache_fixture.cpp').read_text()
    with tempfile.TemporaryDirectory(prefix='sr-texture-check-') as temp:
        temp = Path(temp)
        cpp = temp / 'check.cpp'
        cpp.write_text(fixture.replace('// SDK_METHODS', methods))
        exe = temp / 'check'
        subprocess.run([args.cxx, '-std=c++20', '-O2', '-pthread', '-Wall', '-Wextra',
                        '-Werror', str(cpp), '-o', str(exe)], check=True)
        # The option is read once per process; cover both paths separately.
        subprocess.run([str(exe)], cwd=temp, check=True, timeout=20)
        (temp / 'texture_lookup_cache.off').touch()
        subprocess.run([str(exe)], cwd=temp, check=True, timeout=20)


if __name__ == '__main__':
    main()
