/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.resource.language;

import java.io.IOException;
import java.util.Properties;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

public class TranslationStorage {
    private static TranslationStorage INSTANCE = new TranslationStorage();
    private Properties translations = new Properties();

    private TranslationStorage() {
        try {
            this.translations.load(TranslationStorage.class.getResourceAsStream("/lang/en_US.lang"));
            this.translations.load(TranslationStorage.class.getResourceAsStream("/lang/stats_US.lang"));
        }
        catch (IOException iOException) {
            iOException.printStackTrace();
        }
    }

    public static TranslationStorage getInstance() {
        return INSTANCE;
    }

    public String get(String key) {
        return this.translations.getProperty(key, key);
    }

    public String get(String key, Object ... args) {
        String string = this.translations.getProperty(key, key);
        return String.format(string, args);
    }

    @Environment(value=EnvType.CLIENT)
    public String getClientTranslation(String key) {
        return this.translations.getProperty(key + ".name", "");
    }
}

