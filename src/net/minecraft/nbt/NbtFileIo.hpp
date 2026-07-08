#pragma once
// Crash-safe file persistence shared by every on-disk save (level.dat, player data, chunk
// files, mod/saved-data states). Centralizes the temp-write -> fsync -> atomic-swap dance that
// was previously copy-pasted (and, in some places, missing) across the storage classes.
#include <filesystem>
#include <functional>
#include <ostream>

namespace net::minecraft {
struct AtomicWriteOptions {
    // Force the temp's bytes onto stable storage before the swap. On by default; turn off for
    // hot paths (chunk saves) where per-file fsync would stall the tick and the data is
    // regenerable. A graceful close still flushes the OS cache, so off only loses data on a
    // hard kill / power loss.
    bool fsync = true;
    // Keep "<path>_old" after a successful write so a loader with a fallback (level.dat) always
    // has a recoverable previous copy. Off leaves no clutter in steady state.
    bool keepBackup = false;
};

// Flush a file's cached bytes onto stable storage (Win32 FlushFileBuffers / POSIX fsync).
void fsyncFile(const std::filesystem::path& path);
// Write `path` atomically. `writer` serializes the payload into a sibling temp ("<path>_new");
// the temp is flushed, verified, optionally fsync'd, then swapped into place while "<path>_old"
// holds the previous copy across the swap. Throws (leaving the existing file untouched and the
// temp removed) if the temp can't be opened or the write fails, so an interrupted save never
// destroys the last good copy.
void writeFileAtomic(const std::filesystem::path& path,
                     const std::function<void(std::ostream&)>& writer,
                     const AtomicWriteOptions& options = {});
}  // namespace net::minecraft
