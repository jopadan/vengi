/**
 * @file
 */

#pragma once

#include "core/Path.h"
#include "core/SharedPtr.h"
#include "core/collection/DynamicArray.h"
#include "io/FilesystemEntry.h"
#include "io/Stream.h"

namespace io {

class Filesystem;

using ArchiveFiles = core::DynamicArray<io::FilesystemEntry>;
using FilesystemPtr = core::SharedPtr<Filesystem>;

/**
 * @ingroup IO
 * @sa ZipArchive
 * @sa FilesystemArchive
 */
class Archive {
protected:
	ArchiveFiles _files;

public:
	const ArchiveFiles &files() const;
	/**
	 * @brief List all entities in the archive that match the given optional filter and base directory
	 * @param basePath The directory to list (can be empty)
	 * @param out The list of directory entities that were found
	 * @param[in] filter Wildcard for filtering the returned entities. Separated by a comma. Example *.vox,*.qb,*.mcr
	 */
	virtual void list(const core::String &basePath, ArchiveFiles &out, const core::String &filter) const;
	virtual void list(const core::String &filter, ArchiveFiles &out) const;

	virtual ~Archive() = default;
	virtual bool exists(const core::Path &file) const;
	virtual bool exists(const core::String &file) const;

	/**
	 * @param[in] stream @c io::SeekableReadStream pointer can be @c nullptr
	 */
	virtual bool init(const core::String &path, io::SeekableReadStream *stream);
	/**
	 * @note Shutting down the archive might invalidate all streams that were created by the archive implementation
	 */
	virtual void shutdown();

	/**
	 * @note the default implementation of readStream() uses load() internally
	 * this might not be the most efficient way to read a file from an archive
	 *
	 * The contract is that the memory ownership of the returned stream is transferred to the caller
	 *
	 * @sa core::ScopedPtr
	 */
	virtual SeekableReadStream *readStream(const core::String &filePath) = 0;
	SeekableReadStream *readStream(const core::Path &filePath) {
		return readStream(filePath.toString());
	}
	/**
	 * The contract is that the memory ownership of the returned stream is transferred to the caller
	 *
	 * @sa core::ScopedPtr
	 */
	virtual SeekableWriteStream *writeStream(const core::String &filePath);
	SeekableWriteStream *writeStream(const core::Path &filePath) {
		return writeStream(filePath.toString());
	}

	virtual bool write(const core::String &filePath, io::ReadStream &stream);
};

inline const ArchiveFiles &Archive::files() const {
	return _files;
}

using ArchivePtr = core::SharedPtr<Archive>;
bool isZipArchive(const core::String &filename);

} // namespace io
