#ifndef __PROCESSES_LAYOUT_HPP__
#define __PROCESSES_LAYOUT_HPP__

// Shared geometry for the Processes-tab table. These values were
// previously mirrored as file-local constexprs in coremetrics.cpp,
// MouseEvents.cpp, KeyboardEvents.cpp, and HeadlessRunner.cpp, with
// lockstep-update comments in each; this header is the unification
// those comments promised. The row grid here must stay in sync with
// the <row> and zebra <box> elements in base.xml.
//
// Row pitch is 24px: the 20pt Body face rasterizes a 27px glyph
// surface whose visible ink spans roughly y+4..y+25, so a 24px pitch
// keeps each row's ink clear of the next row while descenders share
// the 3px of dead surface padding. The previous 20px pitch let every
// row's descenders collide with the row below (and the zebra boxes
// painted over them).
namespace ProcessesLayout
{
    constexpr int kFirstRowY = 100;
    constexpr int kRowHeight = 24;
    constexpr int kVisibleRows = 14;
    constexpr int kRowX0 = 24;
    constexpr int kRowX1 = 936;
}

#endif
