#include "TVOverlayManager.h"
#include "ButtonComboManager.h"
#include "export.h"
#include "globals.h"
#include "logger.h"

#include <coreinit/cache.h>
#include <coreinit/debug.h>
#include <coreinit/time.h>

static ButtonComboModule_ComboHandle sTVButtonHandle;
static OSTime sTVPressTime[2];
static bool sTVMenuBlocked[2];

static void setTVMenuBlock(VPADChan channel, bool block)
{
    VPADSetTVMenuInvalid(channel, block);
    sTVMenuBlocked[channel] = block;
}

static void TVComboCallback(ButtonComboModule_ControllerTypes triggeredBy,
                            ButtonComboModule_ComboHandle,
                            void *) {
    VPADChan channel;
    switch (triggeredBy) {
        case BUTTON_COMBO_MODULE_CONTROLLER_VPAD_0:
            channel = VPAD_CHAN_0;
            break;
        case BUTTON_COMBO_MODULE_CONTROLLER_VPAD_1:
            channel = VPAD_CHAN_1;
            break;
        default:
            return;
    }

    // OSReport("TV pressed\n");

    // Note: we need to wait a bit to enable the TV menu, otherwise it shows up on the
    // first press. The sTVPressTime variable is used to keep track of how much time we
    // need to wait.

    sTVPressTime[channel] = OSGetSystemTime();

    OSMemoryBarrier();
}

void registerTVCombo() {
    ButtonComboModule_ComboOptions opt               = {};
    opt.version                                      = BUTTON_COMBO_MODULE_COMBO_OPTIONS_VERSION;
    opt.metaOptions.label                            = "TV remote overlay combo";
    opt.callbackOptions.callback                     = TVComboCallback;
    opt.callbackOptions.context                      = {};
    opt.buttonComboOptions.type                      = BUTTON_COMBO_MODULE_COMBO_TYPE_PRESS_DOWN_OBSERVER;
    opt.buttonComboOptions.basicCombo.combo          = BCMPAD_BUTTON_TV;
    opt.buttonComboOptions.basicCombo.controllerMask = BUTTON_COMBO_MODULE_CONTROLLER_VPAD;
    opt.buttonComboOptions.optionalHoldForXMs        = 0;
    if (ButtonComboModule_AddButtonCombo(&opt, &sTVButtonHandle, nullptr) != BUTTON_COMBO_MODULE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE("FAILED TO SET UP TV COMBO!");
    }
}

void unregisterTVCombo() {
    ButtonComboModule_RemoveButtonCombo(sTVButtonHandle);
}

void initTVStatus(VPADChan channel, bool block) {
    VPADSetTVMenuInvalid(channel, block);
    sTVMenuBlocked[channel] = block;

    sTVPressTime[channel] = 0;
    OSMemoryBarrier();
}

void resetTVStatus(VPADChan channel) {
    sTVPressTime[channel] = 0;
    OSMemoryBarrier();
}

// Called from VPADRead() to time-out the TV button unblock.
void updateTVStatus(VPADChan channel) {

    OSTime pressed = sTVPressTime[channel];
    if (pressed == 0)
        return;

    bool isBlocked = sTVMenuBlocked[channel];
    OSTime elapsed = OSGetSystemTime() - pressed;

    // We wait a bit to enable the TV menu, so it doesn't show up on the first press.
    const OSTime menuDelay = OSMillisecondsToTicks(100);
    if (elapsed >= menuDelay && isBlocked) {
        OSReport("Unblocking the TV menu\n");
        setTVMenuBlock(channel, false);
        return;
    }

    // If too much time passed, and the TV menu is not open, block it again (if needed.)
    const OSTime timeout = OSMillisecondsToTicks(1000);
    if (elapsed > timeout && !VPADGetTVMenuStatus(channel)) {
        bool shouldBlock = gButtonComboManager->hasActiveComboWithTVButton();
        OSReport("TV timeout reached, setting TV Menu to %s\n",
                 shouldBlock ? "blocked" : "unblocked");
        setTVMenuBlock(channel, shouldBlock);
        sTVPressTime[channel] = 0;
    }
}

// Called every time after a combo is added, removed, modified.
void updateTVMenuBlocking()
{
    if (!gButtonComboManager)
        return;
    bool shouldBlock = gButtonComboManager->hasActiveComboWithTVButton();
    setTVMenuBlock(VPAD_CHAN_0, shouldBlock);
    setTVMenuBlock(VPAD_CHAN_1, shouldBlock);

    OSMemoryBarrier();
}
