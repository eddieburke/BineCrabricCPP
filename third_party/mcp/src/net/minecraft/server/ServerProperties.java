/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.server;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.Properties;
import java.util.logging.Level;
import java.util.logging.Logger;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.SERVER)
public class ServerProperties {
    public static Logger logger = Logger.getLogger("Minecraft");
    private Properties properties = new Properties();
    private File propertiesFile;

    public ServerProperties(File file) {
        this.propertiesFile = file;
        if (file.exists()) {
            try {
                this.properties.load(new FileInputStream(file));
            }
            catch (Exception exception) {
                logger.log(Level.WARNING, "Failed to load " + file, exception);
                this.generateNew();
            }
        } else {
            logger.log(Level.WARNING, file + " does not exist");
            this.generateNew();
        }
    }

    public void generateNew() {
        logger.log(Level.INFO, "Generating new properties file");
        this.save();
    }

    public void save() {
        try {
            this.properties.store(new FileOutputStream(this.propertiesFile), "Minecraft server properties");
        }
        catch (Exception exception) {
            logger.log(Level.WARNING, "Failed to save " + this.propertiesFile, exception);
            this.generateNew();
        }
    }

    public String getProperty(String property, String fallback) {
        if (!this.properties.containsKey(property)) {
            this.properties.setProperty(property, fallback);
            this.save();
        }
        return this.properties.getProperty(property, fallback);
    }

    public int getProperty(String property, int fallback) {
        try {
            return Integer.parseInt(this.getProperty(property, "" + fallback));
        }
        catch (Exception exception) {
            this.properties.setProperty(property, "" + fallback);
            return fallback;
        }
    }

    public boolean getProperty(String property, boolean fallback) {
        try {
            return Boolean.parseBoolean(this.getProperty(property, "" + fallback));
        }
        catch (Exception exception) {
            this.properties.setProperty(property, "" + fallback);
            return fallback;
        }
    }

    public void setProperty(String property, boolean value) {
        this.properties.setProperty(property, "" + value);
        this.save();
    }
}

