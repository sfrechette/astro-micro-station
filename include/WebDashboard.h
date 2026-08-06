#pragma once
#include "AstroAPI.h"

// ═══════════════════════════════════════════════════════════════════════════════
//  WebDashboard.h  –  read-only HTTP dashboard (desktop/mobile browser)
//  Serves the same AstroData already held in memory; does not touch the
//  on-device TFT UI or the touch/render loop in any way.
//  Implementation: src/WebDashboard.cpp
// ═══════════════════════════════════════════════════════════════════════════════

void webDashboardBegin(AstroAPI* astro);
