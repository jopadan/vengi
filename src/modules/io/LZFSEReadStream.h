/**
 * @file
 */

#pragma once

#include "Stream.h"
#include "io/MemoryReadStream.h"

namespace io {

/**
 * @ingroup IO
 */
class LZFSEReadStream : public io::ReadStream {
private:
	io::MemoryReadStream *_readStream = nullptr;
	uint8_t *_extractedBuffer = nullptr;

public:
	/**
	 * @param size The compressed size
	 */
	LZFSEReadStream(io::SeekableReadStream &readStream, int size = -1);
	virtual ~LZFSEReadStream();

	/**
	 * @brief Read an arbitrary sized amount of bytes from the input stream
	 *
	 * @param dataPtr The target data buffer
	 * @param dataSize The size of the target data buffer
	 * @return The amount of read bytes or @c -1 on error
	 */
	int read(void *dataPtr, size_t dataSize) override;
	/**
	 * @return @c true if the end of the compressed stream was found
	 */
	bool eos() const override;

	/**
	 * @brief Advances the position in the stream without reading the bytes.
	 * @param delta the bytes to skip
	 * @return -1 on error
	 */
	int64_t skip(int64_t delta);

	/**
	 * @brief The remaining amount of bytes to read from the input stream. This is
	 * either the amount of remaining bytes in the input stream, or if the @c size
	 * parameter was specified in the constructor, the amounts of bytes that are left
	 * relative to the size that was specified.
	 *
	 * @return int64_t
	 */
	int64_t remaining() const;
};

} // namespace io