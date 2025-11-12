#pragma once

#include <tavros/core/memory/buffer_view.hpp>
#include <tavros/core/memory/buffer_span.hpp>

namespace tavros::core
{

    /**
     * @brief Uncompresses data from a compressed buffer into a destination buffer.
     *
     * This function inflates (uncompresses) data stored in @p compressed_data
     * and writes the resulting uncompressed bytes into the provided @p output buffer.
     * The size of the uncompressed data must exactly match @p output.size(),
     * otherwise the operation may fail or produce incomplete output.
     *
     * @param compressed_data  View of the compressed data to uncompress.
     *                         Its size is obtained via `compressed_data.size()`.
     * @param output           Destination buffer where the uncompressed bytes will be written.
     *                         Must be preallocated and large enough to hold the full uncompressed data.
     *
     * @return `true` if uncompression succeeds, or `false` if zlib reports an error
     *         (e.g., corrupted input or size mismatch).
     */
    bool uncompress_data(buffer_view<uint8> compressed_data, buffer_span<uint8> output);

} // namespace tavros::core
