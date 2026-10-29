# SPDX-License-Identifier: GPL-2.0-only

"""
Common Pixel constants.
"""

load("//common:modules.bzl", "get_gki_modules_list")

def _map_pixel_gki_module(module, map_each):
    # Pixel builds its own zram module.
    if module == "drivers/block/zram/zram.ko":
        return None
    return map_each(module) if map_each else module

def get_pixel_gki_modules_list(map_each = None):
    """Return the configurable GKI module list for Pixel.

    Args:
        map_each: Optional function to transform each retained module path.
    """
    return get_gki_modules_list(
        "arm64",
        map_each = lambda module: _map_pixel_gki_module(module, map_each),
    )
