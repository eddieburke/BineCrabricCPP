/*
 * Decompiled with CFR 0.152.
 */
package com.seibel.distanthorizons.api;

import com.seibel.distanthorizons.api.interfaces.config.IDhApiConfig;
import com.seibel.distanthorizons.api.interfaces.data.IDhApiTerrainDataRepo;
import com.seibel.distanthorizons.api.interfaces.events.IDhApiEventInjector;
import com.seibel.distanthorizons.api.interfaces.factories.IDhApiWrapperFactory;
import com.seibel.distanthorizons.api.interfaces.override.IDhApiOverrideable;
import com.seibel.distanthorizons.api.interfaces.override.worldGenerator.IDhApiWorldGeneratorOverrideRegister;
import com.seibel.distanthorizons.api.interfaces.render.IDhApiCustomRenderObjectFactory;
import com.seibel.distanthorizons.api.interfaces.render.IDhApiRenderProxy;
import com.seibel.distanthorizons.api.interfaces.world.IDhApiWorldProxy;
import com.seibel.distanthorizons.api.methods.override.DhApiWorldGeneratorOverrideRegister;
import com.seibel.distanthorizons.coreapi.DependencyInjection.ApiEventInjector;
import com.seibel.distanthorizons.coreapi.DependencyInjection.OverrideInjector;
import com.seibel.distanthorizons.coreapi.ModInfo;
import com.seibel.distanthorizons.coreapi.interfaces.dependencyInjection.IOverrideInjector;

public class DhApi {
    public static final String READ_ME = "If you don't see Javadocs something is wrong. \nIf you are only using the full DH Mod in your build script, you won't have access to our javadocs and could potentially call into unsafe code. \n\nPlease use the API jar in your build script as a compile time dependency and the full DH jar as a runtime dependency. \n\nPlease refer to the example API project or the DH Developer Wiki for additional information and suggested setup. \n";
    public static final String HEY_YOU_YOURE_FINALLY_AWAKE = "If you don't see Javadocs something is wrong. \nIf you are only using the full DH Mod in your build script, you won't have access to our javadocs and could potentially call into unsafe code. \n\nPlease use the API jar in your build script as a compile time dependency and the full DH jar as a runtime dependency. \n\nPlease refer to the example API project or the DH Developer Wiki for additional information and suggested setup. \n";
    public static final IDhApiEventInjector events = ApiEventInjector.INSTANCE;
    public static final IDhApiWorldGeneratorOverrideRegister worldGenOverrides = DhApiWorldGeneratorOverrideRegister.INSTANCE;
    public static final IOverrideInjector<IDhApiOverrideable> overrides = OverrideInjector.INSTANCE;

    public static String readMe() {
        return "If you don't see Javadocs something is wrong. \nIf you are only using the full DH Mod in your build script, you won't have access to our javadocs and could potentially call into unsafe code. \n\nPlease use the API jar in your build script as a compile time dependency and the full DH jar as a runtime dependency. \n\nPlease refer to the example API project or the DH Developer Wiki for additional information and suggested setup. \n";
    }

    public static String heyYou_YoureFinallyAwake() {
        return "If you don't see Javadocs something is wrong. \nIf you are only using the full DH Mod in your build script, you won't have access to our javadocs and could potentially call into unsafe code. \n\nPlease use the API jar in your build script as a compile time dependency and the full DH jar as a runtime dependency. \n\nPlease refer to the example API project or the DH Developer Wiki for additional information and suggested setup. \n";
    }

    public static int getApiMajorVersion() {
        return 3;
    }

    public static int getApiMinorVersion() {
        return 0;
    }

    public static int getApiPatchVersion() {
        return 0;
    }

    public static String getModVersion() {
        return "2.1.3-a-dev";
    }

    public static boolean getIsDevVersion() {
        return ModInfo.IS_DEV_BUILD;
    }

    public static int getNetworkProtocolVersion() {
        return 1;
    }

    public static boolean isDhThread() {
        return Thread.currentThread().getName().startsWith("DH-");
    }

    public static class Delayed {
        public static IDhApiConfig configs = null;
        public static IDhApiTerrainDataRepo terrainRepo = null;
        public static IDhApiWorldProxy worldProxy = null;
        public static IDhApiRenderProxy renderProxy = null;
        public static IDhApiWrapperFactory wrapperFactory = null;
        public static IDhApiCustomRenderObjectFactory customRenderObjectFactory = null;
    }
}

