import os
from typing import Literal as _


# Build name
# os.environ.setdefault("BUILD_NAME", "dev")

# Target platform
platform:_["egl", "server", "x11"]
platform = "egl"

# Build the tools (a.k.a. the Godot editor)
tools:_[True, False]
tools = False

# Compilation target
target:_["debug", "release_debug", "release"]
target = "release_debug"

# Platform-dependent architecture
arch:_["arm", "arm64", "x86", "x64", "mips"]

# Target platform bits
bits:_["default", "32", "64"]

# Optimization type
optimize:_["speed", "size", "none"]

# Set defaults to build Godot for use in production
production:_[False, True]

# Add debugging symbols to release/release_debug builds
debug_symbols:_[True, False]
debug_symbols = True

# Create a separate file containing debugging symbols
separate_debug_symbols:_[False, True]
separate_debug_symbols = True

# Enable verbose output for the compilation
verbose:_[False, True]
verbose = False

# Use link-time optimization
use_lto:_[False, True]
use_lto = False

# Enable deprecated features
deprecated:_[True, False]
deprecated = True

# Enable GDScript support
gdscript:_[True, False]

# Enable ZIP archive support using minizip
minizip:_[True, False]
minizip = True

# Enable the XAudio2 audio driver
xaudio2:_[False, True]
xaudio2 = False

# Detect and use PulseAudio
pulseaudio:_[True, False]
pulseaudio = False

# Enable touch events
touch:_[True, False]
touch = False

# Use udev for gamepad connection callbacks
udev:_[True, False]
udev = False

# A list of comma-separated directory paths containing custom modules to build.
custom_modules: str

# Detect custom modules recursively for each specified path.
custom_modules_recursive:_[True, False]

# If yes, alias for verbose=yes warnings=extra werror=yes
dev:_[False, True]

# Show a progress indicator during compilation
progress:_[True, False]

# Level of compilation warnings
warnings:_["all", "extra", "moderate", "no"]

# Treat compiler warnings as errors
werror:_[False, True]

# Custom extra suffix added to the base filename of all generated binary files
extra_suffix: str

# Generate a Visual Studio solution
vsproj:_[False, True]

# Split intermediate libmodules.a in smaller chunks to prevent exceeding linker command line size
split_libmodules:_[False, True]

# Disable 3D nodes for a smaller executable
disable_3d:_[False, True]

# Disable advanced GUI nodes and behaviors
disable_advanced_gui:_[False, True]
disable_advanced_gui = False

# Don't use the custom splash screen for the editor
no_editor_splash:_[False, True]
no_editor_splash = True

# Use this path as SSL certificates default for editor (for package maintainers)
system_certs_path: str

# Math checks use very precise epsilon (debug option)
use_precise_math_checks:_[False, True]

# Use libexecinfo on systems where glibc is not available
execinfo:_[False, True]


# Use the LLVM compiler
use_llvm:_[False, True]

# Use the LLD linker
use_lld:_[False, True]

# Use ThinLTO
use_thinlto:_[False, True]

# Link libgcc and libstdc++ statically for better portability
use_static_cpp:_[True, False]
use_static_cpp = False

# Use LLVM/GCC compiler undefined behavior sanitizer (UBSAN)
use_ubsan:_[False, True]

# Use LLVM/GCC compiler address sanitizer (ASAN))
use_asan:_[False, True]

# Use LLVM/GCC compiler leak sanitizer (LSAN))
use_lsan:_[False, True]

# Use LLVM/GCC compiler thread sanitizer (TSAN))
use_tsan:_[False, True]

# Use LLVM/GCC compiler memory sanitizer (MSAN))
use_msan:_[False, True]


module_bullet_enabled = True
module_camera_enabled = True
module_csg_enabled = True
module_freetype_enabled = True
module_gdnative_enabled = True
module_gdscript_enabled = True
module_raycast_enabled = True
module_regex_enabled = True

module_mbedtls_enabled = True
module_svg_enabled = True

module_bmp_enabled = False
module_cvtt_enabled = False
module_dds_enabled = False
module_denoise_enabled = False
module_enet_enabled = False
module_etc_enabled = False
module_fbx_enabled = False
module_gridmap_enabled = False
module_hdr_enabled = False
module_jpg_enabled = False
module_jsonrpc_enabled = False
module_lightmapper_cpu_enabled = False
module_minimp3_enabled = False
module_mobile_vr_enabled = False
module_mono_enabled = False
module_ogg_enabled = False
module_opensimplex_enabled = False
module_opus_enabled = False
module_pvr_enabled = False
module_recast_enabled = False
module_squish_enabled = False
module_stb_vorbis_enabled = False
module_tga_enabled = False
module_theora_enabled = False
module_tinyexr_enabled = True
module_upnp_enabled = False
module_vhacd_enabled = False
module_visual_script_enabled = False
module_vorbis_enabled = False
module_webm_enabled = False
module_webp_enabled = False
module_webrtc_enabled = False
module_websocket_enabled = False
module_webxr_enabled = False
module_xatlas_unwrap_enabled = False


builtin_bullet:_[True, False]
builtin_certs:_[True, False]
builtin_embree:_[True, False]
builtin_enet:_[True, False]
builtin_freetype:_[True, False]
builtin_libogg:_[True, False]
builtin_libpng:_[True, False]
builtin_libtheora:_[True, False]
builtin_libvorbis:_[True, False]
builtin_libvpx:_[True, False]
builtin_libwebp:_[True, False]
builtin_wslay:_[True, False]
builtin_mbedtls:_[True, False]
builtin_miniupnpc:_[True, False]
builtin_opus:_[True, False]
builtin_pcre2:_[True, False]
builtin_pcre2_with_jit:_[True, False]
builtin_recast:_[True, False]
builtin_squish:_[True, False]
builtin_xatlas:_[True, False]
builtin_zlib:_[True, False]
builtin_zstd:_[True, False]
