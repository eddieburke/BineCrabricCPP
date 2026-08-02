/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api.interfaces.config.client;

import com.seibel.distanthorizons.api.enums.config.EDhApiServerFolderNameMode;
import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfigGroup;
import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfigValue;

public interface IDhApiMultiplayerConfig
extends IDhApiConfigGroup {
    public IDhApiConfigValue<EDhApiServerFolderNameMode> folderSavingMode();

    public IDhApiConfigValue<Double> multiverseSimilarityRequirement();
}

