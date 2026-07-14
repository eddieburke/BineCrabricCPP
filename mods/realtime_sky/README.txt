REAL-TIME SKY — accuracy and stability replacement

Install
=======
Replace the mod's main.lua and scripts directory with the files in this package.
Keep the existing assets/cities.json and assets/globe_coasts.txt files.

Major corrections
=================
- The rendered sun now uses the calculated apparent altitude and azimuth.
  Latitude and season therefore affect the noon height correctly.
- Minecraft's 0.00 sunrise / 0.25 noon / 0.50 sunset / 0.75 midnight phase is
  kept separate from the physical sun zenith angle.
- The mod no longer rewrites the server/world clock every tick.
- Settings are loaded into the existing shared table instead of replacing it.
- Fixed-offset parsing works for GMT/UTC offsets, half-hour and quarter-hour
  zones, and reversed-sign Etc/GMT identifiers.
- Common IANA zones use explicit DST rules. Unknown named zones fall back to a
  longitude-derived standard offset instead of silently becoming UTC.
- cities.json failures now fall back to scripts/places.lua.
- Globe horizontal dragging and initial camera framing are corrected.
- Twilight, fog, and star brightness now transition continuously.
- The globe overlay reports the actual current solar azimuth and altitude.

Validation performed
====================
- Every Lua file passes texluac syntax checking.
- Solar-position reconstruction was tested for Toronto summer/winter and
  Greenwich equinox samples.
- Fixed-offset, IANA-zone, DST, and simulated-local-time conversions were
  checked with deterministic test dates.

Limits
======
This package does not embed the complete IANA timezone database. Common zones
are mapped explicitly; an unrecognized named zone is marked approximate and
uses longitude to choose a standard offset. Actual rendering still needs an
in-engine test because the Minecraft host API itself is unavailable here.
