#pragma once

namespace tavros::resources
{

    /**
     * @brief Represents the direction for seeking within a stream or file.
     */
    enum class seek_dir
    {
        /// Seek relative to the beginning of the stream.
        begin,

        /// Seek relative to the current position in the stream.
        current,

        /// Seek relative to the end of the stream.
        end,
    };

} // namespace tavros::resources