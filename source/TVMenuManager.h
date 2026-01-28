#pragma once

#include <vpad/input.h>

namespace TVMenuManager {
    
    void registerCombo();

    void unregisterCombo();

    void init(VPADChan channel, bool block);

    void reset(VPADChan channel);

    void update(VPADChan channel);

    void updateBlockState();
    
} // namespace TVMenuManager
