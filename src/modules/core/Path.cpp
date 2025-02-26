/**
 * @file
 */

#include "core/Path.h"
#include "core/Common.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include <SDL_platform.h>

namespace core {

Path::Path(const core::String &path) : _path(path) {
	core::string::replaceAllChars(_path, '\\', '/');
}

Path::Path(const core::String &path1, const core::String &path2) {
	_path = core::string::path(path1, path2);
	core::string::replaceAllChars(_path, '\\', '/');
}

Path::Path(core::String &&path) : _path(core::move(path)) {
	core::string::replaceAllChars(_path, '\\', '/');
}

char Path::separator() const {
#if defined(_WIN32) || defined(__CYGWIN__)
	return '\\';
#else
	return '/';
#endif
}

core::String Path::toNativePath() const {
#if defined(_WIN32) || defined(__CYGWIN__)
	core::String path = _path;
	core::string::replaceAllChars(path, '/', '\\');
	return path;
#else
	return _path;
#endif
}

char Path::driveLetter() const {
	if (_path.size() >= 2 && _path[1] == ':') {
		return core::string::toUpper(_path[0]);
	}
#if defined(_WIN32) || defined(__CYGWIN__)
	return 'C';
#else
	return '/';
#endif
}

bool Path::hasParentDirectory() const {
	if (isRootPath()) {
		return false;
	}
	return _path.find("/") != core::String::npos;
}

Path Path::dirname() const {
	core::String base = _path;
	if (base.size() == 1 && base.last() == '/') {
		return core::Path(base);
	}
	while (base.last() == '/') {
		base.erase(base.size() - 1);
	}
	const size_t pos = base.find_last_of("/");
	if (pos == core::String::npos) {
		return Path(".");
	}
	return core::Path(base.substr(0, pos));
}

Path Path::basename() const {
	core::String base = _path;
	if (base.size() == 1 && base.last() == '/') {
		return core::Path(base);
	}
	while (base.last() == '/') {
		base.erase(base.size() - 1);
	}
	const size_t pos = base.find_last_of("/");
	if (pos != core::String::npos) {
		base = base.substr(pos + 1);
	}
	return core::Path(base);
}

core::String Path::extension() const {
	const size_t pos = _path.find_last_of(".");
	if (pos == core::String::npos) {
		return core::String();
	}
	if (_path.find_last_of("/") > pos) {
		return core::String();
	}
	return _path.substr(pos + 1);
}

Path Path::removeExtension() const {
	const size_t pos = _path.find_last_of(".");
	if (pos == core::String::npos) {
		return *this;
	}
	if (_path.find_last_of("/") > pos) {
		return *this;
	}
	return core::Path(_path.substr(0, pos));
}

Path Path::replaceExtension(const core::String &newExtension) const {
	const size_t pos = _path.find_last_of(".");
	if (pos == core::String::npos || _path.find_last_of("/") > pos) {
		return core::Path(_path + "." + newExtension);
	}
	return core::Path(_path.substr(0, pos) + "." + newExtension);
}

bool Path::isRelativePath() const {
	const size_t size = _path.size();
#if defined(_WIN32) || defined(__CYGWIN__)
	if (size < 2) {
		return true;
	}
	// TODO: hm... not cool and most likely not enough
	return _path[1] != ':';
#else
	if (size == 0) {
		return true;
	}
	return _path[0] != '/';
#endif
}

bool Path::isAbsolutePath() const {
	if (_path.size() >= 3U && core::string::isAlpha(_path[0]) && _path[1] == ':' &&
		(_path[2] == '\\' || _path[2] == '/')) {
		return true;
	}
	return _path.size() > 1U && (_path[0] == '/' || _path[0] == '\\');
}

bool Path::isRootPath() const {
	if (_path.size() == 3U && core::string::isAlpha(_path[0]) && _path[1] == ':' &&
		(_path[2] == '\\' || _path[2] == '/')) {
		return true;
	}
	return _path.size() == 1U && (_path[0] == '/' || _path[0] == '\\');
}

core::DynamicArray<core::String> Path::components() const {
	core::DynamicArray<core::String> c;
	core::string::splitString(_path, c, "/");
	return c;
}

Path Path::append(const core::String &component) const {
	return Path(core::string::path(_path, component));
}

Path Path::append(const core::Path &component) const {
	return Path(core::string::path(_path, component.str()));
}

Path Path::popFront() const {
	const core::String &p = lexicallyNormal();
	const size_t pos = p.find_first_of("/");
	if (pos == core::String::npos) {
		return Path();
	}
	return Path(p.substr(pos + 1));
}

Path Path::popBack() const {
	const core::String &p = lexicallyNormal();
	const size_t pos = p.find_last_of("/");
	if (pos == core::String::npos) {
		return Path();
	}
	return Path(p.substr(0, pos));
}

core::String Path::lexicallyNormal() const {
	core::DynamicArray<core::String> parts;
	const bool is_absolute = isAbsolutePath();

	core::String driveLetter;

	// Handle drive letters for Windows paths
	const core::DynamicArray<core::String> &c = components();
	if (is_absolute && c.size() > 0) {
		const core::String &firstComponent = c[0];
		// Detect drive letter (e.g., "C:")
		if (firstComponent.size() == 2 && firstComponent[1] == ':') {
			driveLetter = firstComponent;
		}
	}

	for (const core::String &component : c) {
		if (component.empty() || component == ".") {
			continue;
		}
		if (component == "..") {
			if (!parts.empty() && parts.back() != "..") {
				parts.pop();
			} else if (!is_absolute) {
				// Keep ".." if it's at the beginning of a relative path
				parts.push_back("..");
			}
		} else if (component != driveLetter) { // Avoid re-adding the drive letter
			parts.push_back(component);
		}
	}
	core::String newPath;
	// Append drive letter if present (Windows absolute paths)
	if (!driveLetter.empty()) {
		newPath += driveLetter;
		newPath += "/";
	} else if (is_absolute) {
		newPath += "/";
	}

	for (size_t i = 0; i < parts.size(); ++i) {
		if (i > 0) {
			newPath += "/";
		}
		newPath += parts[i];
	}
	return newPath;
}

Path &Path::operator+=(const core::String &other) {
	_path = core::string::path(_path, other);
	return *this;
}

Path &Path::operator+=(const Path &other) {
	_path = core::string::path(_path, other._path);
	return *this;
}

bool Path::operator==(const core::String &other) const {
	core::String otherCleaned = other;
	core::string::replaceAllChars(otherCleaned, '\\', '/');
	if (otherCleaned.last() == '/') {
		otherCleaned.erase(otherCleaned.size() - 1);
	}
	core::String ownCleaned = _path;
	if (ownCleaned.last() == '/') {
		ownCleaned.erase(ownCleaned.size() - 1);
	}
	return ownCleaned == otherCleaned;
}

bool Path::operator!=(const core::String &other) const {
	return !(*this == other);
}

bool Path::operator==(const Path &other) const {
	return *this == other._path;
}

bool Path::operator!=(const Path &other) const {
	return *this != other._path;
}

Path operator+(const Path &lhs, const core::Path &rhs) {
	return Path(core::string::path(lhs.str(), rhs.str()));
}

Path operator+(const Path &lhs, const core::String &rhs) {
	return Path(core::string::path(lhs.str(), rhs));
}

} // namespace core
