# Tavros

Tavros is a personal project developed by me as a learning and exploration exercise.  
It originally started as an idea for building a custom game engine. Over time, the concept evolved into a modular system with the following core components:

- **Cross-platform rendering**
- **Custom core library**
- **Cross-platform windowing system**

---

## Build Instructions

The project uses a Python-based build script. To build Tavros, you will need:

- **Python** 3.9 or higher
- **CMake** 3.20 or higher
- **clang-format** 21.1

To start the build process, run:

```bash
./tavros.py
```

The script will provide detailed instructions and additional build options.

## Project Structure

The repository is organized as follows:

- **docs/** – Project documentation
- **tests/** – All unit tests
- **sources/** – Engine source code (excluding tests)
- **sources/libs/** – Custom libraries (rendering, core, audio, etc.)
- **sources/third/** – Third-party libraries
- **sources/wrappers/** – Example wrappers or demos

## Features

### Current
- **Core library**
  - Custom containers (`array`, `map`, `vector`, `unordered_map`, `unordered_set`, etc.)
  - Debugging utilities (`assert`, `verify`, debug break, unreachable)
  - Geometry primitives (`aabb2`, `aabb3`, `plane`, `ray3`, `sphere`, `obb3`) with intersection and distance functions
  - Math module with vectors, matrices, quaternions, euler angles, and a rich set of functions (dot, cross, normalization, lerp, slerp, determinant, inverse, etc.)
  - Memory management (`allocator`, `zone_allocator`, `mallocator`)
  - Logger with severity levels
  - Scoped and RAII helpers (`scoped_owner`, `optional`, `noncopyable`, `nonmovable`, `pimpl`)
  - Timing utilities (`timer`)
  - Resource pool abstraction

- **Renderer**
  - Cross-platform rendering interface (RHI layer: buffers, textures, samplers, pipelines, render passes, frame composers, vertex layouts, etc.)
  - OpenGL backend (initial implementation with command list, graphics device, context management)

- **System**
  - Cross-platform windowing system (Windows, macOS)
  - Window state management
  - Event callbacks (keyboard, window events, etc.)
  - Application abstraction (`application` interface with run/exit/event polling)
  - Key mapping with platform-independent key codes

### Planned
- Additional rendering backends (Vulkan, Metal, DirectX)
- Audio subsystem
- Asset/resource management layer
- More advanced scene management
- Cross-platform input handling (mouse, gamepad, touch)
- High-level abstractions for GUI and gameplay systems
- Example applications and demos
- Comprehensive test coverage and benchmarks

## License

*The license will be added later.*
For now, all source code may be used "as is."
The author does not take any responsibility for potential issues arising from its use.

## Author

Kulbaka Piotr Alexandrovich
