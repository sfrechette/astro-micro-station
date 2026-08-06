#pragma once
#include "AstroAPI.h"

// ═══════════════════════════════════════════════════════════════════════════════
//  WebDashboard.h  –  read-only HTTP dashboard (desktop/mobile browser)
//  Serves the same AstroData already held in memory; does not touch the
//  on-device TFT UI or the touch/render loop in any way.
//  Implementation: src/WebDashboard.cpp
// ═══════════════════════════════════════════════════════════════════════════════

void webDashboardBegin(AstroAPI* astro);

// True exactly once after the web UI saves a new observer location. The
// network fetch that applies it happens from loop() on the main task, not
// from the async server's own task — call this each loop() iteration and
// re-fetch when it returns true.
bool webDashboardConsumeLocationChange();
