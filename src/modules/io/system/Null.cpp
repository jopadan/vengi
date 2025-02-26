/**
 * @file
 */

#include <SDL_platform.h>

#if !defined(__linux__) && !defined(__APPLE__) && !defined(_WIN32) && !defined(__CYGWIN__) && !defined(__EMSCRIPTEN__)
#include "io/Filesystem.h"

namespace io {

bool initState(io::FilesystemState &state) {
	return false;
}

bool fs_mkdir(const char *path) {
	return false;
}

bool fs_rmdir(const char *path) {
	return false;
}

bool fs_hidden(const char *path) {
	return false;
}

bool fs_unlink(const char *path) {
	return false;
}

bool fs_exists(const char *path) {
	return false;
}

bool fs_writeable(const char *path) {
	return false;
}

bool fs_chdir(const char *path) {
	return false;
}

core::String fs_realpath(const char *path) {
	return path;
}

bool fs_stat(const char *path, FilesystemEntry &entry) {
	return false;
}

core::DynamicArray<FilesystemEntry> fs_scandir(const char *path) {
	core::DynamicArray<FilesystemEntry> foo;
	return foo;
}

core::String fs_readlink(const char *path) {
	return path;
}

core::String fs_cwd() {
	return "/";
}

} // namespace io

#endif
