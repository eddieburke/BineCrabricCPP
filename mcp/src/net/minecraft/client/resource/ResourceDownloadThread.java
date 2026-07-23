/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.resource;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.net.URL;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.Minecraft;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

@Environment(value=EnvType.CLIENT)
public class ResourceDownloadThread
extends Thread {
    public File resourcesDirectory;
    private Minecraft minecraft;
    private boolean cancelled = false;

    public ResourceDownloadThread(File runDirectory, Minecraft minecraft) {
        this.minecraft = minecraft;
        this.setName("Resource download thread");
        this.setDaemon(true);
        this.resourcesDirectory = new File(runDirectory, "resources/");
        if (!this.resourcesDirectory.exists() && !this.resourcesDirectory.mkdirs()) {
            throw new RuntimeException("The working directory could not be created: " + this.resourcesDirectory);
        }
    }

    public void run() {
        try {
            URL uRL = new URL("http://s3.amazonaws.com/MinecraftResources/");
            DocumentBuilderFactory documentBuilderFactory = DocumentBuilderFactory.newInstance();
            DocumentBuilder documentBuilder = documentBuilderFactory.newDocumentBuilder();
            Document document = documentBuilder.parse(uRL.openStream());
            NodeList nodeList = document.getElementsByTagName("Contents");
            for (int i = 0; i < 2; ++i) {
                for (int j = 0; j < nodeList.getLength(); ++j) {
                    Node node = nodeList.item(j);
                    if (node.getNodeType() != 1) continue;
                    Element element = (Element)node;
                    String string = ((Element)element.getElementsByTagName("Key").item(0)).getChildNodes().item(0).getNodeValue();
                    long l = Long.parseLong(((Element)element.getElementsByTagName("Size").item(0)).getChildNodes().item(0).getNodeValue());
                    if (l <= 0L) continue;
                    this.loadFromUrl(uRL, string, l, i);
                    if (!this.cancelled) continue;
                    return;
                }
            }
        }
        catch (Exception exception) {
            this.loadFromDirectory(this.resourcesDirectory, "");
            exception.printStackTrace();
        }
    }

    public void reload() {
        this.loadFromDirectory(this.resourcesDirectory, "");
    }

    private void loadFromDirectory(File directory, String type) {
        File[] fileArray = directory.listFiles();
        for (int i = 0; i < fileArray.length; ++i) {
            if (fileArray[i].isDirectory()) {
                this.loadFromDirectory(fileArray[i], type + fileArray[i].getName() + "/");
                continue;
            }
            try {
                this.minecraft.loadResource(type + fileArray[i].getName(), fileArray[i]);
                continue;
            }
            catch (Exception exception) {
                System.out.println("Failed to add " + type + fileArray[i].getName());
            }
        }
    }

    private void loadFromUrl(URL url, String path, long size, int type) {
        try {
            int n = path.indexOf("/");
            String string = path.substring(0, n);
            if (string.equals("sound") || string.equals("newsound") ? type != 0 : type != 1) {
                return;
            }
            File file = new File(this.resourcesDirectory, path);
            if (!file.exists() || file.length() != size) {
                file.getParentFile().mkdirs();
                String string2 = path.replaceAll(" ", "%20");
                this.downloadFile(new URL(url, string2), file, size);
                if (this.cancelled) {
                    return;
                }
            }
            this.minecraft.loadResource(path, file);
        }
        catch (Exception exception) {
            exception.printStackTrace();
        }
    }

    private void downloadFile(URL url, File destinationFile, long size) {
        byte[] byArray = new byte[4096];
        DataInputStream dataInputStream = new DataInputStream(url.openStream());
        DataOutputStream dataOutputStream = new DataOutputStream(new FileOutputStream(destinationFile));
        int n = 0;
        while ((n = dataInputStream.read(byArray)) >= 0) {
            dataOutputStream.write(byArray, 0, n);
            if (!this.cancelled) continue;
            return;
        }
        dataInputStream.close();
        dataOutputStream.close();
    }

    public void cancel() {
        this.cancelled = true;
    }
}

