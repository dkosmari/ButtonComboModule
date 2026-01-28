#pragma once

#include <vpad/input.h>

namespace TVMenuManager {
    
    void registerCombo();

    void unregisterCombo();

    void updateTimer(VPADChan channel);

    void updateBlockState(VPADChan channel);

    void updateBlockState();
    
} // namespace TVMenuManager
