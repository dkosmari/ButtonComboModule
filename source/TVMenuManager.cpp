#include "TVMenuManager.h"
#include "ButtonComboManager.h"
#include "export.h"
#include "globals.h"
#include "logger.h"

#include <coreinit/cache.h>
#include <coreinit/debug.h>
#include <coreinit/time.h>

namespace TVMenuManager {

    namespace {

        static ButtonComboModule_ComboHandle sComboHandle;
        // Note: we need to wait a bit to enable the TV menu, otherwise it shows up on the
        // first press. The sTVPressTime variable is used to keep track of how much time
        // has passedsince the last TV button press.
        static OSTime sPressTime[2];
        static bool sIsBlocked[2];

        static void setBlockState(VPADChan channel, bool block)
        {
            VPADSetTVMenuInvalid(channel, block);
            sIsBlocked[channel] = block;
        }

        void TVComboCallback(ButtonComboModule_ControllerTypes triggeredBy,
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

            sPressTime[channel] = OSGetSystemTime();

            OSMemoryBarrier();
        }

    } // namespace

    void registerCombo() {
        ButtonComboModule_ComboOptions opt               = {};
        opt.version                                      = BUTTON_COMBO_MODULE_COMBO_OPTIONS_VERSION;
        opt.metaOptions.label                            = "TV remote overlay combo";
        opt.callbackOptions.callback                     = TVComboCallback;
        opt.callbackOptions.context                      = {};
        opt.buttonComboOptions.type                      = BUTTON_COMBO_MODULE_COMBO_TYPE_PRESS_DOWN_OBSERVER;
        opt.buttonComboOptions.basicCombo.combo          = BCMPAD_BUTTON_TV;
        opt.buttonComboOptions.basicCombo.controllerMask = BUTTON_COMBO_MODULE_CONTROLLER_VPAD;
        opt.buttonComboOptions.optionalHoldForXMs        = 0;
        if (ButtonComboModule_AddButtonCombo(&opt, &sComboHandle, nullptr) != BUTTON_COMBO_MODULE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE("FAILED TO SET UP TV COMBO!");
        }
    }

    void unregisterCombo() {
        ButtonComboModule_RemoveButtonCombo(sComboHandle);
    }

    void init(VPADChan channel, bool block) {
        setBlockState(channel, block);
        reset(channel);
    }

    void reset(VPADChan channel) {
        sPressTime[channel] = 0;
        OSMemoryBarrier();
    }

    // Called from VPADRead() to timeout the TV menu unblock.
    void update(VPADChan channel) {

        OSTime pressed = sPressTime[channel];
        if (pressed == 0)
            return;

        bool isBlocked = sIsBlocked[channel];
        OSTime elapsed = OSGetSystemTime() - pressed;

        // We wait a bit to enable the TV menu, so it doesn't show up on the first press.
        const OSTime menuDelay = OSMillisecondsToTicks(500);
        if (elapsed >= menuDelay && isBlocked) {
            OSReport("Unblocking the TV menu\n");
            setBlockState(channel, false);
            OSMemoryBarrier();
            return;
        }

        // If too much time passed, and the TV menu is not open, block it again (if needed.)
        const OSTime timeout = OSMillisecondsToTicks(500);
        if (elapsed > timeout && !VPADGetTVMenuStatus(channel)) {
            bool shouldBlock = gButtonComboManager->hasActiveComboWithTVButton();
            OSReport("TV timeout reached, setting TV Menu to %s\n",
                     shouldBlock ? "blocked" : "unblocked");
            setBlockState(channel, shouldBlock);
            sPressTime[channel] = 0;
            OSMemoryBarrier();
        }
    }

    // Called every time after a combo is added, removed, modified.
    void updateBlockState()
    {
        if (!gButtonComboManager)
            return;
        bool shouldBlock = gButtonComboManager->hasActiveComboWithTVButton();
        setBlockState(VPAD_CHAN_0, shouldBlock);
        setBlockState(VPAD_CHAN_1, shouldBlock);

        OSMemoryBarrier();
    }

} // namespace TVMenuManager
