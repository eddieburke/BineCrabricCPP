/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.enums.config;

import com.seibel.distanthorizons.coreapi.ModInfo;

public enum EDhApiUpdateBranch {
    AUTO,
    STABLE,
    NIGHTLY;


    public static EDhApiUpdateBranch convertAutoToStableOrNightly(EDhApiUpdateBranch updateBranch) {
        if (updateBranch != AUTO) {
            return updateBranch;
        }
        return ModInfo.IS_DEV_BUILD ? NIGHTLY : STABLE;
    }
}

